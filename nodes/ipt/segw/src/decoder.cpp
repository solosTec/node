/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "decoder.h"
#include "cache.h"

#include <smf/mbus/defs.h>
#include <smf/mbus/header.h>
#include <smf/mbus/aes.h>
#include <smf/mbus/variable_data_block.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/protocol/parser.h>

#include <cyng/crypto/aes_keys.h>
#include <cyng/io/io_buffer.h>
#include <cyng/io/serializer.h>
#ifdef _DEBUG
#include <cyng/io/hex_dump.hpp>
#endif

namespace node
{
	decoder_wired_mbus::decoder_wired_mbus(cyng::logging::log_ptr logger)
		: logger_(logger)
		, uuidgen_()
	{}

	decoder_wireless_mbus::decoder_wireless_mbus(cyng::logging::log_ptr logger
		, cache& cfg
		, cyng::controller& vm)
	: logger_(logger)
		, cache_(cfg)
		, vm_(vm)
		, uuidgen_()
	{}

	/*
	 * The first 12 bytes of the user data consist of a block with a fixed size and structure.
	 *
	 * Applied from master with CI = 0x53, 0x55, 0x5B, 0x5F, 0x60, 0x64, 0x6Ch, 0x6D
	 * Applied from slave with CI = 0x68, 0x6F, 0x72, 0x75, 0x7C, 0x7E, 0x9F
	 */
	bool decoder_wireless_mbus::read_frame_header_long(cyng::buffer_t const& srv, cyng::buffer_t const& data)
	{
		auto r1 = make_header_long(1, data);
		if (r1.second) {

			CYNG_LOG_DEBUG(logger_, "access nr   : " << +r1.first.header().get_access_no());

			auto const server_id = r1.first.get_srv_id();
			auto const manufacturer = sml::get_manufacturer_code(server_id);
			auto const mbus_status = r1.first.header().get_status();	//	OBIS_MBUS_STATE (00 00 61 61 00 FF)

			//
			//	0, 5, 7, or 13
			//	currently only 0 (== no encryption) and 5 (== AES CBC) is supported
			//
			auto const aes_mode = r1.first.header().get_mode();

			CYNG_LOG_DEBUG(logger_, "server ID   : " << sml::from_server_id(server_id));	//	OBIS_SERIAL_NR - 00 00 60 01 00 FF
			CYNG_LOG_DEBUG(logger_, "medium      : " << mbus::get_medium_name(sml::get_medium_code(server_id)));
			CYNG_LOG_DEBUG(logger_, "manufacturer: " << sml::decode(manufacturer));	//	OBIS_DATA_MANUFACTURER - 81 81 C7 82 03 FF
			CYNG_LOG_DEBUG(logger_, "device id   : " << sml::get_serial(server_id));
			CYNG_LOG_DEBUG(logger_, "status      : " << +mbus_status);
			CYNG_LOG_DEBUG(logger_, "mode        : " << +aes_mode);

			//
			// update device table
			//
			if (cache_.update_device_table(server_id
				, sml::decode(manufacturer)
				, mbus_status
				, sml::get_version(server_id)
				, sml::get_medium_code(server_id)
				, cyng::crypto::aes_128_key{}
				, vm_.tag())) {

				CYNG_LOG_INFO(logger_, "new device: "
					<< sml::from_server_id(server_id));
			}

			auto r2 = this->decode_data(aes_mode, server_id, r1.first);
			if (r2.second) {

				//
				//	populate cache with new readout
				//
				auto const pk = uuidgen_();

				//
				//	read data block
				//
				CYNG_LOG_DEBUG(logger_, "decoded data: "
					<< cyng::io::to_hex(r2.first.header().data_raw()));

				read_variable_data_block(server_id, r2.first.header(), pk);

				//
				//	"_Readout"
				//
				cache_.write_table("_Readout", [&](cyng::store::table* tbl) {
					tbl->insert(cyng::table::key_generator(pk)
						, cyng::table::data_generator(server_id, std::chrono::system_clock::now(), mbus_status)
						, 1u	//	generation
						, cache_.get_tag());
				});

			}
			else {

				CYNG_LOG_WARNING(logger_, "cannot read long header of " << sml::from_server_id(srv));
			}
			return r2.second;
		}
		return false;
	}

	/**
	 * Applied from slave with CI = 0x7A
	 */
	bool decoder_wireless_mbus::read_frame_header_short(cyng::buffer_t const& srv, cyng::buffer_t const& data)
	{
		auto r1 = make_header_short(data);
		if (r1.second) {
			CYNG_LOG_DEBUG(logger_, "access nr: " << +r1.first.get_access_no());

			auto const manufacturer = sml::get_manufacturer_code(srv);
			auto const mbus_status = r1.first.get_status();

			//
			//	0, 5, 7, or 13
			//	currently only 0 (== no encryption) and 5 (== AES CBC) is supported
			//
			auto const aes_mode = r1.first.get_mode();

			CYNG_LOG_DEBUG(logger_, "manufacturer: " << manufacturer);
			CYNG_LOG_DEBUG(logger_, "status      : " << +mbus_status);
			CYNG_LOG_DEBUG(logger_, "mode        : " << +aes_mode);

			auto r2 = decode_data(aes_mode, srv, r1.first);
			if (r2.second) {

				//
				//	read data block
				//
				CYNG_LOG_DEBUG(logger_, "decoded data: "
					<< cyng::io::to_hex(r2.first.data()));

				//
				//	ToDo: read
				//
			}
			else {
				CYNG_LOG_WARNING(logger_, "cannot read short header (CI = 0x7A) of " << sml::from_server_id(srv));
			}

			return r2.second;
		}
		return false;
	}

	bool decoder_wireless_mbus::read_frame_header_short_sml(cyng::buffer_t const& srv_id, cyng::buffer_t const& data)
	{
		auto r1 = make_header_short(data);
		if (r1.second) {

			auto const manufacturer = sml::get_manufacturer_code(srv_id);

			//
			//	0, 5, 7, or 13
			//	currently only 0 (== no encryption) and 5 (== AES CBC) is supported
			//
			auto const aes_mode = r1.first.get_mode();
			auto const mbus_status = r1.first.get_status();

			CYNG_LOG_DEBUG(logger_, sml::decode(manufacturer)
				<< ": "
				<< mbus::get_medium_name(sml::get_medium_code(srv_id))
				<< " - "
				<< sml::from_server_id(srv_id)
				<< std::hex
				<< ", access #0x"
				<< +r1.first.get_access_no()
				<< ", status: 0x"
				<< +mbus_status
				<< ", mode: "
				<< std::dec
				<< +aes_mode
				<< ", counter: "
				<< +(r1.first.get_block_counter() * 16u)	//	 V * 16 Bytes
			);

			//
			//	encrypt data
			//
			auto r2 = decode_data(aes_mode, srv_id, r1.first);
			if (r2.second) {

#ifdef __DEBUG
				cyng::io::hex_dump hd;
				std::stringstream ss;
				hd(ss, r2.first.data_raw().begin(), r2.first.data_raw().end());

				CYNG_LOG_TRACE(logger_, "SML data:\n"
					<< ss.str());

#endif
				//
				//	populate cache with new readout
				//
				auto const pk = uuidgen_();

				//
				//	"_Readout"
				//
				cache_.write_table("_Readout", [&](cyng::store::table* tbl) {
					tbl->insert(cyng::table::key_generator(pk)
						, cyng::table::data_generator(srv_id, std::chrono::system_clock::now(), mbus_status)
						, 1u	//	generation
						, cache_.get_tag());
				});

				//
				//	start SML parser
				//
				sml::parser sml_parser([&](cyng::vector_t&& prg) {

					//
					//	execute "sml.msg" and "sml.eom" instructions
					//
					//CYNG_LOG_TRACE(logger_, "SML instructions: "
					//	<< cyng::io::to_str(prg));

					vm_.async_run(std::move(prg));

				}, false, false, false, pk);	//	not verbose, no logging
				sml_parser.read(r2.first.data_raw().begin() + 2, r2.first.data_raw().end());
			}
			else {
				CYNG_LOG_WARNING(logger_, "cannot read short header (CI = 0x7F) of " << sml::from_server_id(srv_id));
			}

			return r2.second;
		}
		return false;
	}

	std::pair<header_long, bool> decoder_wireless_mbus::decode_data(std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_long& hl)
	{
		switch (aes_mode) {
		case 0:	return std::make_pair(header_long(std::move(hl)), true);	//	not decrypted
		case 5:	return decode_data_mode_5(aes_mode, server_id, hl);
		case 7:
		case 13:
		default:
			break;
		}

		CYNG_LOG_ERROR(logger_, "AES mode "
			<< +aes_mode
			<< " is not supported - server ID: "
			<< sml::from_server_id(server_id));

		return std::make_pair(header_long(std::move(hl)), false);
	}

	std::pair<header_short, bool> decoder_wireless_mbus::decode_data(std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_short& hs)
	{
		switch (aes_mode) {
		case 0:	return std::make_pair(header_short(std::move(hs)), true);	//	not decrypted
		case 5:	return decode_data_mode_5(aes_mode, server_id, hs);
		case 7:
		case 13:
		default:
			break;
		}

		CYNG_LOG_ERROR(logger_, "AES mode "
			<< +aes_mode
			<< " is not supported - server ID: "
			<< sml::from_server_id(server_id));

		return std::make_pair(header_short(std::move(hs)), false);
	}

	std::pair<header_long, bool> decoder_wireless_mbus::decode_data_mode_5(std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_long& hl)
	{
		//
		//	get AES key and decrypt content (AES CBC - mode 5)
		//
		cyng::crypto::aes_128_key key = cache_.get_aes_key(server_id);

		//
		//	decode payload data
		//
		auto r = decode(hl, key);
		if (!r.second) {
			CYNG_LOG_WARNING(logger_, "meter "
				<< sml::from_server_id(server_id)
				<< " has invalid AES key: "
				<< cyng::io::to_hex(key.to_buffer(), ' '));

			return std::make_pair(header_long(), false);
		}

		bool const b = r.first.header().verify_encryption();
		return std::make_pair(header_long(std::move(r.first)), b);
	}

	std::pair<header_short, bool> decoder_wireless_mbus::decode_data_mode_5(std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_short& hs)
	{
		//
		//	get AES key and decrypt content (AES CBC - mode 5)
		//
		cyng::crypto::aes_128_key key = cache_.get_aes_key(server_id);

		//
		//	build initialization vector
		//
		auto const iv = mbus::build_initial_vector(server_id, hs.get_access_no());

		//
		//	decode payload data
		//
		auto r = decode(hs, key, iv);
		if (!r.second) {
			CYNG_LOG_WARNING(logger_, "meter "
				<< sml::from_server_id(server_id)
				<< " has invalid AES key: "
				<< cyng::io::to_hex(key.to_buffer(), ' '));

			return std::make_pair(hs, false);
		}

		bool const b = r.first.verify_encryption();
		return std::make_pair(std::move(r.first), b);
	}

	void decoder_wireless_mbus::read_variable_data_block(cyng::buffer_t const& server_id
		, header_short const& hs
		, boost::uuids::uuid pk)
	{
		//
		//	get number of encrypted bytes
		//
		auto const blocks = hs.get_block_counter();
		if (0 == blocks) {

			//	not encrypted
		}
		else if (0x0F == blocks) {
			// partial encryption disabled
		}
		else {
			//
			//	multiples of 16
			//
			auto size = 16u * static_cast<std::size_t>(blocks);

			CYNG_LOG_DEBUG(logger_, "read_variable_data_block("
				<< size
				<< "/"
				<< hs.data_raw().size()
				<< " bytes, meter "
				<< sml::from_server_id(server_id)
				<< ")");

			vdb_reader reader(server_id);
			std::size_t offset{ 2 };	//	2 x 0x2F
			while (offset < size) {

				//
				//	read block
				//
				offset = reader.decode(hs.data_raw(), offset);

				//
				//	cache: "_ReadoutData"
				//
				if (reader.is_valid()) {

					cache_.write_table("_ReadoutData", [&](cyng::store::table* tbl) {

						auto val = cyng::io::to_str(reader.get_value());
						auto type = static_cast<std::uint32_t>(reader.get_value().get_class().tag());

						tbl->insert(cyng::table::key_generator(pk, reader.get_code().to_buffer())
							, cyng::table::data_generator(val, type, reader.get_scaler(), static_cast<std::uint8_t>(reader.get_unit()))
							, 1u	//	generation
							, cache_.get_tag());
					});
				}
			}
		}
	}
}

