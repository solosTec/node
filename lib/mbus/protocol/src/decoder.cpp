/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include <smf/mbus/decoder.h>
#include <smf/mbus/defs.h>
#include <smf/mbus/variable_data_block.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/protocol/parser.h>

#include <cyng/vm/controller.h>

namespace node
{
	namespace mbus
	{
	}
	namespace wmbus
	{
		decoder::decoder(cyng::logging::log_ptr logger
			, cyng::controller& vm
			, cb_meter cb1
			, cb_data cb2
			, cb_value cb3)
		: logger_(logger)
			, vm_(vm)
			, cb_meter_(cb1)
			, cb_data_(cb2)
			, cb_value_(cb3)
			, uuidgen_()
		{}

		bool decoder::run(cyng::buffer_t const& srv_id
			, std::uint8_t frame_type
			, cyng::buffer_t const& payload
			, cyng::crypto::aes_128_key const& aes)
		{
			switch (frame_type) {
			case node::mbus::FIELD_CI_HEADER_LONG:	//	0x72
				//	e.g. HYD
				return read_frame_header_long(srv_id, payload, aes);
			case node::mbus::FIELD_CI_HEADER_SHORT:	//	0x7A
				return read_frame_header_short(srv_id, payload, aes);
			case node::mbus::FIELD_CI_RES_SHORT_SML:	//	0x7F
				//	e.g. ED300L
				return read_frame_header_short_sml(srv_id, payload, aes);
			default:
				break;
			}

			return false;
		}

		bool decoder::read_frame_header_long(cyng::buffer_t const& srv_id
			, cyng::buffer_t const& payload
			, cyng::crypto::aes_128_key const& aes)
		{
			//	1 = wireless
			auto r1 = make_header_long_wireless(payload);
			if (r1.second) {

				auto const server_id = r1.first.get_srv_id();
				auto const mbus_status = r1.first.get_status();	//	OBIS_MBUS_STATE (00 00 61 61 00 FF)

				//
				//	0, 5, 7, or 13
				//	currently only 0 (== no encryption) and 5 (== AES CBC) is supported
				//
				auto const aes_mode = r1.first.get_mode();

				//
				//	meter callback
				//
				cb_meter_(server_id, mbus_status, aes_mode, aes);

				auto const prefix = r1.first.get_prefix();
				auto r2 = decode_data(server_id, prefix, aes);
				if (r2.second) {

					//
					//	new readout pk
					//
					auto const pk = uuidgen_();

					//
					//	data callback
					//
					cb_data_(server_id, r2.first, mbus_status, pk);

					switch (r1.first.get_block_counter()) {
					case 0:
						//	not encrypted
						break;
					case 0x0f:
						// partial encryption disabled
						break;
					default:
						read_variable_data_block(server_id
							, r1.first.get_block_counter()
							, r2.first
							, pk);
					}
					return true;
				}
			}
			return false;
		}

		bool decoder::read_frame_header_short(cyng::buffer_t const& srv_id
			, cyng::buffer_t const& payload
			, cyng::crypto::aes_128_key const& aes)
		{
			auto const prefix = make_wireless_prefix(payload.begin(), payload.end());
			if (prefix.second) {
				auto const manufacturer = sml::get_manufacturer_code(srv_id);
				CYNG_LOG_DEBUG(logger_, "access nr: " << +prefix.first.get_access_no());
				CYNG_LOG_DEBUG(logger_, "manufacturer: " << manufacturer);
				CYNG_LOG_DEBUG(logger_, "status      : " << +prefix.first.get_status());	//	mBus status
				CYNG_LOG_DEBUG(logger_, "mode        : " << +prefix.first.get_mode());	//	AES mode
			}
			return false;
		}

		bool decoder::read_frame_header_short_sml(cyng::buffer_t const& srv_id
			, cyng::buffer_t const& payload
			, cyng::crypto::aes_128_key const& aes)
		{
			auto const prefix = make_wireless_prefix(payload.begin(), payload.end());
			if (prefix.second) {
				auto const manufacturer = sml::get_manufacturer_code(srv_id);
				CYNG_LOG_DEBUG(logger_, sml::decode(manufacturer)
					<< ": "
					<< mbus::get_medium_name(sml::get_medium_code(srv_id))
					<< " - "
					<< sml::from_server_id(srv_id)
					<< std::hex
					<< ", access #0x"
					<< +prefix.first.get_access_no()
					<< ", status: 0x"
					<< +prefix.first.get_status()
					<< ", mode: "
					<< std::dec
					<< +prefix.first.get_mode()
					<< ", counter: "
					<< +prefix.first.blocks()	//	 V * 16 Bytes
				);

				//
				//	encrypt data
				//
				auto r2 = decode_data(srv_id, prefix.first, aes);
				if (r2.second) {

					//
					//	new readout
					//
					auto const pk = uuidgen_();

					//
					//	data callback
					//
					cb_data_(srv_id, r2.first, prefix.first.get_status(), pk);

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

						//
						//	parse raw data
						//
					sml_parser.read(r2.first.begin() + 2, r2.first.end());

				}
			}
			return false;
		}

		std::pair<cyng::buffer_t, bool> decoder::decode_data(cyng::buffer_t const& server_id
			, wireless_prefix const& prefix
			, cyng::crypto::aes_128_key const& aes)
		{
			switch (prefix.get_mode()) {
			case 0:	return std::make_pair(prefix.get_data(), true);	//	not decrypted
			case 5:	return prefix.decode_mode_5(server_id, aes);
			case 7:
			case 13:
			default:
				break;
			}

			CYNG_LOG_ERROR(logger_, "AES mode "
				<< +prefix.get_mode()
				<< " is not supported - server ID: "
				<< sml::from_server_id(server_id));

			return std::make_pair(cyng::buffer_t{}, false);
		}

		void decoder::read_variable_data_block(cyng::buffer_t const& server_id
			, std::uint8_t const blocks
			, cyng::buffer_t const& raw_data
			, boost::uuids::uuid pk)
		{
			BOOST_ASSERT(blocks != 0);
			BOOST_ASSERT(blocks != 0x0f);

			auto size = 16u * static_cast<std::size_t>(blocks);

			CYNG_LOG_DEBUG(logger_, "read_variable_data_block("
				<< size
				<< "/"
				<< raw_data.size()
				<< " bytes, meter "
				<< sml::from_server_id(server_id)
				<< ")");

			vdb_reader reader(server_id);
			std::size_t offset{ 2 };	//	2 x 0x2F

			BOOST_ASSERT(raw_data.size() != 0);
			BOOST_ASSERT(raw_data.at(0) == 0x2f);
			BOOST_ASSERT(raw_data.at(1) == 0x2f);

			while (offset < size) {

				//
				//	read block
				//
				offset = reader.decode(raw_data, offset);

				if (reader.is_valid()) {

					cb_value_(server_id
						, reader.get_value()
						, reader.get_scaler()
						, reader.get_unit()
						, reader.get_code()
						, pk);
				}
			}
		}
	}
}


