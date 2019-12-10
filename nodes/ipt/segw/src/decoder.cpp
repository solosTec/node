/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "decoder.h"
#include "cache.h"
#include <smf/mbus/header.h>
#include <smf/sml/srv_id_io.h>
#include <smf/mbus/aes.h>
#include <smf/mbus/variable_data_block.h>

#include <cyng/crypto/aes_keys.h>
#include <cyng/io/io_buffer.h>

namespace node
{
	decoder_wired_mbus::decoder_wired_mbus(cyng::logging::log_ptr logger)
		: logger_(logger)
	{}

	decoder_wireless_mbus::decoder_wireless_mbus(cyng::logging::log_ptr logger, cache& cfg, boost::uuids::uuid tag)
		: logger_(logger)
		, cache_(cfg)
		, tag_(tag)
	{}

	/*
	 * The first 12 bytes of the user data consist of a block with a fixed length and structure.
	 *
	 * Applied from master with CI = 0x53, 0x55, 0x5B, 0x5F, 0x60, 0x64, 0x6Ch, 0x6D
	 * Applied from slave with CI = 0x68, 0x6F, 0x72, 0x75, 0x7C, 0x7E, 0x9F
	 */
	bool decoder_wireless_mbus::read_frame_header_long(cyng::buffer_t const& srv, cyng::buffer_t const& data)
	{
		auto r = make_header_long(1, data);
		if (r.second) {

			CYNG_LOG_DEBUG(logger_, "access nr   : " << +r.first.header().get_access_no());

			auto const server_id = r.first.get_srv_id();
			auto const manufacturer = sml::get_manufacturer_code(server_id);
			auto const mbus_status = r.first.header().get_status();

			//
			//	0, 5, 7, or 13
			//	currently only 0 (== no encryption) and 5 (== AES CBC) is supported
			//
			auto const aes_mode = r.first.header().get_mode();

			CYNG_LOG_DEBUG(logger_, "server ID   : " << sml::from_server_id(server_id));
			CYNG_LOG_DEBUG(logger_, "manufacturer: " << sml::decode(manufacturer));
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
				, tag_)) {

				CYNG_LOG_INFO(logger_, "new device: "
					<< sml::from_server_id(server_id));
			}

			if (decode_data(aes_mode, server_id, r.first)) {

				//
				//	read data block
				//
				CYNG_LOG_DEBUG(logger_, "decoded data: "
					<< cyng::io::to_hex(r.first.header().data()));

				read_variable_data_block(server_id, r.first.header());


			}
			else {

				CYNG_LOG_WARNING(logger_, "cannot read long header of " << sml::from_server_id(srv));
			}
		}
		return r.second;
	}

	/**
	 * Applied from slave with CI = 0x7A
	 */
	bool decoder_wireless_mbus::read_frame_header_short(cyng::buffer_t const& srv, cyng::buffer_t const& data)
	{
		auto r = make_header_short(data);
		if (r.second) {
			CYNG_LOG_DEBUG(logger_, "access nr: " << +r.first.get_access_no());

			auto const manufacturer = sml::get_manufacturer_code(srv);
			auto const mbus_status = r.first.get_status();

			//
			//	0, 5, 7, or 13
			//	currently only 0 (== no encryption) and 5 (== AES CBC) is supported
			//
			auto const aes_mode = r.first.get_mode();

			CYNG_LOG_DEBUG(logger_, "manufacturer: " << manufacturer);
			CYNG_LOG_DEBUG(logger_, "status      : " << +mbus_status);
			CYNG_LOG_DEBUG(logger_, "mode        : " << +aes_mode);

			if (decode_data(aes_mode, srv, r.first)) {

				//
				//	read data block
				//
				CYNG_LOG_DEBUG(logger_, "decoded data: "
					<< cyng::io::to_hex(r.first.data()));
			}
			else {

				CYNG_LOG_WARNING(logger_, "cannot read short header of " << sml::from_server_id(srv));
			}

		}
		return r.second;
	}

	bool decoder_wireless_mbus::read_frame_header_short_sml(cyng::buffer_t const& srv, cyng::buffer_t const& data)
	{
		auto const r = make_header_short(data);
		if (r.second) {

			auto const manufacturer = sml::get_manufacturer_code(srv);

			//
			//	0, 5, 7, or 13
			//	currently only 0 (== no encryption) and 5 (== AES CBC) is supported
			//
			auto const aes_mode = r.first.get_mode();

			CYNG_LOG_DEBUG(logger_, sml::decode(manufacturer)
				<< ": "
				<< mbus::get_medium_name(sml::get_medium_code(srv))
				<< " - "
				<< sml::from_server_id(srv)
				<< std::hex
				<< ", access #"
				<< +r.first.get_access_no()
				<< ", status: "
				<< +r.first.get_status()
				<< ", mode: "
				<< std::dec
				<< +aes_mode
				<< ", counter: "
				<< +(r.first.get_block_counter() * 16u)	//	 V * 16 Bytes
			);

		}
		return r.second;
	}

	bool decoder_wireless_mbus::decode_data(std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_long& hl)
	{
		switch (aes_mode) {
		case 0:	return true;	//	not decrypted
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

		return false;
	}

	bool decoder_wireless_mbus::decode_data(std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_short& hs)
	{
		switch (aes_mode) {
		case 0:	return true;	//	not decrypted
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

		return false;
	}

	bool decoder_wireless_mbus::decode_data_mode_5(std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_long& hl)
	{
		//
		//	get AES key and decrypt content (AES CBC - mode 5)
		//
		cyng::crypto::aes_128_key key = cache_.get_aes_key(server_id);

		//
		//	decode payload data
		//
		if (!hl.decode(key)) {
			CYNG_LOG_WARNING(logger_, "meter "
				<< sml::from_server_id(server_id)
				<< " has invalid AES key: "
				<< cyng::io::to_hex(key.to_buffer(), ' '));
		}

		return hl.header().verify_encryption();
	}

	bool decoder_wireless_mbus::decode_data_mode_5(std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_short& hs)
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
		if (!hs.decode(key, iv)) {
			CYNG_LOG_WARNING(logger_, "meter "
				<< sml::from_server_id(server_id)
				<< " has invalid AES key: "
				<< cyng::io::to_hex(key.to_buffer(), ' '));
		}

		return hs.verify_encryption();

	}

	void decoder_wireless_mbus::read_variable_data_block(cyng::buffer_t const& server_id, header_short const& hs)
	{
		//
		//	get number of encrypted bytes
		//
		auto const size = hs.get_block_counter();
		if (0 == size) {

			//	not encrypted
		}
		else if (0x0F == size) {
			// partial encryption disabled
		}
		else {
			auto length = static_cast<std::size_t>(16u * size);

			//
			//	remove trailing 0x2F
			//
			//length -= hs.remove_aes_trailer();

			CYNG_LOG_DEBUG(logger_, "read_variable_data_block("
				<< length
				<< " bytes, meter "
				<< sml::from_server_id(server_id)
				<< ")");

			vdb_reader reader;
			std::size_t offset{ 0 };
			while (offset < length) {

				//
				//	read block
				//
				std::size_t const new_offset = reader.decode(hs.data(), offset);
				if (new_offset > offset) {
				}
				else {

					//
					//	no more data blocks
					//
					//offset = length;
					break;

				}
			}

		}

	}

}

