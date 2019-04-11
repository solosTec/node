/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "data.h"
#include <smf/mbus/defs.h>
#include <smf/mbus/header.h>
#include <smf/mbus/variable_data_block.h>
#include <smf/mbus/aes.h>
#include <smf/sml/protocol/parser.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_db.h>

#include <cyng/tuple_cast.hpp>
#include <cyng/io/serializer.h>
#include <cyng/io/hex_dump.hpp>
#include <cyng/vm/generator.h>

#include <boost/algorithm/string.hpp>

namespace node
{
	namespace sml
	{

		data::data(cyng::logging::log_ptr logger
			, cyng::controller& vm
			, cyng::store::db& config_db)
		: logger_(logger)
			, config_db_(config_db)
			, params_()
		{
			//
			//	callback from wireless LMN
			//
			vm.register_function("wmbus.push.frame", 7, std::bind(&data::wmbus_push_frame, this, std::placeholders::_1));

			//
			//	data from SML parser after receiving a 0x7F frame (short SML header)
			//
			vm.register_function("sml.get.list.response", 9, std::bind(&data::sml_get_list_response, this, std::placeholders::_1));

			//
			//	process latest received data (this->params_)
			//
			vm.register_function("gw.data.store", 2, std::bind(&data::store, this, std::placeholders::_1));
		}

		void data::sml_get_list_response(cyng::context& ctx)
		{
			//	
			//	example:
			//	[c450540c-b69a-4a89-ae48-017e301f7dff,.,00000000,,01A815743145040102,990000000003,
			//	%(	("0100000009FF":%(("scaler":0),("unit":0),("valTime":null),("value":01A815743145040102))),
			//		("0100010800FF":%(("scaler":-1),("unit":1e),("valTime":null),("value":1452.1))),
			//		("0100010801FF":%(("scaler":-1),("unit":1e),("valTime":null),("value":0))),
			//		("0100010802FF":%(("scaler":-1),("unit":1e),("valTime":null),("value":1452.1))),
			//		("0100020800FF":%(("scaler":-1),("unit":1e),("valTime":null),("value":55330.2))),
			//		("0100020801FF":%(("scaler":-1),("unit":1e),("valTime":null),("value":0))),
			//		("0100020802FF":%(("scaler":-1),("unit":1e),("valTime":null),("value":55330.2))),
			//		("0100100700FF":%(("scaler":-1),("unit":1b),("valTime":null),("value":0))),
			//		("8181C78203FF":%(("scaler":0),("unit":0),("valTime":null),("value":EMH))),
			//		("8181C78205FF":%(("scaler":0),("unit":0),("valTime":null),("value":1C661D023F438BB639D3D95AA580F63DF78F2EA4692709F3D40209C35E98CDBC25B95A7C3A813F55E13AA2DC61020FA2)))),
			//		null,null]
			//
			//	* [uuid] pk (meta data)
			//	* [string] trx
			//	* [size] idx
			//	* [buffer] client id (empty)
			//	* [buffer] server id
			//	* [buffer] OBIS code
			//	* [obj] values
			//	* [ ] actSensorTime
			//	* [ ] actGatewayTime
			//	
			cyng::vector_t const frame = ctx.get_frame();
			//CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//
			//	get server/meter id
			//
			cyng::buffer_t server_id;
			server_id = cyng::value_cast(frame.at(4), server_id);
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << from_server_id(server_id));

			//
			//	extract parameter list with values
			//
			params_.clear();
			params_ = cyng::value_cast(frame.at(6), params_);

			for (auto const& p : params_) {

				CYNG_LOG_DEBUG(logger_, p.first << " = " << cyng::io::to_str(p.second));

				if (boost::algorithm::equals(p.first, OBIS_DATA_PUBLIC_KEY.to_str())) {

					//
					//	update public key
					//
					CYNG_LOG_TRACE(logger_, "update public key " << cyng::io::to_str(p.second));

					config_db_.modify("mbus-devices"
						, cyng::table::key_generator(server_id)
						, cyng::param_t("pubKey", p.second)
						, ctx.tag());
				}
			}

			//
			//	handle special parameters
			//
		}


		void data::wmbus_push_frame(cyng::context& ctx)
		{
			//
			//	incoming wireless M-Bus data
			//

			cyng::vector_t const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			//	[01242396072000630E,HYD,63,e,0003105c,72,29436587E61EBF03B900200540C83A80A8E0668CAAB369804FBEFBA35725B34A55369C7877E42924BD812D6D]
			auto const tpl = cyng::tuple_cast<
				cyng::buffer_t,		//	[0] server id
				std::string,		//	[1] manufacturer
				std::uint8_t,		//	[2] version
				std::uint8_t,		//	[3] media
				std::uint32_t,		//	[4] device id
				std::uint8_t,		//	[5] frame_type
				cyng::buffer_t		//	[6] payload
			>(frame);

			//
			//	extract server ID and payload
			//
			cyng::buffer_t const& server_id = std::get<0>(tpl);
			cyng::buffer_t const& payload = std::get<6>(tpl);

			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - server id: " << from_server_id(server_id));
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - manufacturer: " << std::get<1>(tpl));
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - version: " << +std::get<2>(tpl));
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - media: " << +std::get<3>(tpl) << " - " << node::mbus::get_medium_name(std::get<3>(tpl)));
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - device id: " << std::get<4>(tpl));
			CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - frame type: " << +std::get<5>(tpl));

			//
			//	decoding depends on frame type and AES key from
			//	device table
			//
			switch (std::get<5>(tpl)) {
			case node::mbus::FIELD_CI_HEADER_LONG:	//	0x72
				//	e.g. HYD
				read_frame_header_long(ctx, server_id, payload);
				break;
			case node::mbus::FIELD_CI_HEADER_SHORT:	//	0x7A
				read_frame_header_short(ctx, server_id, payload);
				break;
			case node::mbus::FIELD_CI_RES_SHORT_SML:	//	0x7F
				//	e.g. ED300L
				read_frame_header_short_sml(ctx, server_id, payload);
				break;
			default:
			{
				//
				//	frame type not supported
				//
				std::stringstream ss;
				ss
					<< " frame type ["
					<< std::hex
					<< std::setw(2)
					<< +std::get<5>(tpl)
					<< "] not supported - data\n"
					;

				cyng::io::hex_dump hd;
				hd(ss, std::get<6>(tpl).cbegin(), std::get<6>(tpl).cend());

				CYNG_LOG_WARNING(logger_, ctx.get_name()
					<< ss.str());
			}
			//
			// update device table
			//
			update_device_table(std::get<0>(tpl)
				, std::get<1>(tpl)
				, std::get<2>(tpl)
				, std::get<3>(tpl)
				, std::get<5>(tpl)
				, cyng::crypto::aes_128_key{}
			, ctx.tag());
			break;
			}

		}

		void data::update_device_table(cyng::buffer_t const& dev_id
			, std::string const& manufacturer
			, std::uint8_t version
			, std::uint8_t media
			, std::uint8_t frame_type
			, cyng::crypto::aes_128_key aes_key
			, boost::uuids::uuid tag)
		{
			config_db_.access([&](cyng::store::table* tbl) {

				auto const rec = tbl->lookup(cyng::table::key_generator(dev_id));
				if (rec.empty()) {

					//
					//	new device
					//
					CYNG_LOG_INFO(logger_, "insert new device: " << cyng::io::to_hex(dev_id));

					tbl->insert(cyng::table::key_generator(dev_id)
						, cyng::table::data_generator(std::chrono::system_clock::now()
							, "+++"	//	class
							//, true	//	visible
							, false	//	active
							, manufacturer	//	description
							, 0ull	//	status
							, cyng::buffer_t{ 0, 0 }	//	mask
							, 26000ul	//	interval
							, cyng::make_buffer({})	//	pubKey
							, aes_key	//	 AES key 
							, ""	//	user
							, "")	//	password
						, 1	//	generation
						, tag);

				}
				else {
					CYNG_LOG_TRACE(logger_, "update device: " << cyng::io::to_hex(dev_id));
					tbl->modify(rec.key(), cyng::param_factory("lastSeen", std::chrono::system_clock::now()), tag);
				}
			}, cyng::store::write_access("mbus-devices"));
		}

		void data::read_frame_header_long(cyng::context& ctx, cyng::buffer_t const& srv_id, cyng::buffer_t const& data)
		{
			CYNG_LOG_DEBUG(logger_, "make_header_long: " << cyng::io::to_hex(data));

			std::pair<header_long, bool> r = make_header_long(1, data);
			if (r.second) {

// 				std::cerr << "access nr: " << std::endl;
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


				if (decode_data(ctx.tag(), aes_mode, server_id, r.first)) {

					read_variable_data_block(server_id, r.first.header());

				}
				else {
					CYNG_LOG_WARNING(logger_, "meter " << sml::from_server_id(server_id) << " encryption failed");
				}
			}
		}

		void data::read_frame_header_short(cyng::context& ctx, cyng::buffer_t const& server_id, cyng::buffer_t const& data)
		{
			CYNG_LOG_DEBUG(logger_, "make_header_short " << cyng::io::to_hex(data));

			std::pair<header_short, bool> r = make_header_short(data);
			if (r.second) {
				std::cerr << "access nr: " << std::endl;
				CYNG_LOG_DEBUG(logger_, "access nr: " << +r.first.get_access_no());

				auto const manufacturer = sml::get_manufacturer_code(server_id);
				auto const mbus_status = r.first.get_status();

				//
				//	0, 5, 7, or 13
				//	currently only 0 (== no encryption) and 5 (== AES CBC) is supported
				//
				auto const aes_mode = r.first.get_mode();

				CYNG_LOG_DEBUG(logger_, "manufacturer: " << manufacturer);
				CYNG_LOG_DEBUG(logger_, "status      : " << +mbus_status);
				CYNG_LOG_DEBUG(logger_, "mode        : " << +aes_mode);

				if (decode_data(ctx.tag(), aes_mode, server_id, r.first)) {

					//
					//	read data block
					//
					read_variable_data_block(server_id, r.first);
				}

			}
			else {
				CYNG_LOG_WARNING(logger_, "cannot read short header of " << sml::from_server_id(server_id));
			}
		}

		void data::read_frame_header_short_sml(cyng::context& ctx, cyng::buffer_t const& srv_id, cyng::buffer_t const& data)
		{
			std::pair<header_short, bool> r = make_header_short(data);
			auto const manufacturer = sml::get_manufacturer_code(srv_id);

			//
			//	0, 5, 7, or 13
			//	currently only 0 (== no encryption) and 5 (== AES CBC) is supported
			//
			auto const aes_mode = r.first.get_mode();
			

			if (r.second) {

				std::cout
					<< sml::decode(manufacturer)
					<< ": "
					<< mbus::get_medium_name(sml::get_medium_code(srv_id))
					<< " - "
					<< sml::from_server_id(srv_id)
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
					<< std::endl
					;

				//
				//	encrypt data
				//
				if (decode_data(ctx.tag(), aes_mode, srv_id, r.first)) {

					//
					//	remove trailing 0x2F
					//
					r.first.remove_aes_trailer();

					//
					//	start SML parser
					//
					parser sml_parser([&](cyng::vector_t&& prg) {

						//
						//	This results in a call of sml_get_list_response()
						//
						ctx.queue(std::move(prg));

					}, false, false);	//	not verbose, no logging

					auto const sml = r.first.data();
					sml_parser.read(sml.begin(), sml.end());

					//
					//	process values stored in this->params_
					//
					ctx.queue(cyng::generate_invoke("gw.data.store", srv_id, static_cast<std::uint8_t > (node::mbus::FIELD_CI_RES_SHORT_SML)));
				}
			}
			else {
				CYNG_LOG_WARNING(logger_, "cannot read short header of " << sml::from_server_id(srv_id));
			}
		}

		void data::read_variable_data_block(cyng::buffer_t const& server_id, header_short& hs)
		{
			//
			//	get number of encrypted bytes
			//
			auto counter = hs.get_block_counter() * 16u;

			//
			//	remove trailing 0x2F
			//
			counter -= hs.remove_aes_trailer();

			CYNG_LOG_DEBUG(logger_, "read_variable_data_block("
				<< counter
				<< " bytes, meter "
				<< sml::from_server_id(server_id)
				<< ")");


			vdb_reader reader;
			std::size_t offset{ 0 };
			while (offset < counter) {

				//
				//	read block
				//
				std::size_t const new_offset = reader.decode(hs.data(), offset);
				if (new_offset > offset) {

					//
					//	store block
					//
					CYNG_LOG_INFO(logger_, "offset: "
						<< offset
						<< ", meter "
						<< sml::from_server_id(server_id)
						<< ", value: "
						<< cyng::io::to_str(reader.get_value())
						<< ", scaler: "
						<< +reader.get_scaler()
						<< ", unit: "
						<< get_unit_name(reader.get_unit()));
				}
				else {

					//
					//	no more data blocks
					//
					offset = counter;
					break;
				}
			}
		}

		bool data::decode_data(boost::uuids::uuid tag, std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_long& hl)
		{
			switch (aes_mode) {
			case 0:	return true;	//	not decrypted
			case 5:	return decode_data_mode_5(tag, aes_mode, server_id, hl);
			case 7:
			case 13:
			default:
				break;
			}

			CYNG_LOG_ERROR(logger_, "AES mode "
				<< +aes_mode
				<< " is not supported - server ID: "
				<< from_server_id(server_id));

			return false;
		}

		bool data::decode_data(boost::uuids::uuid tag, std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_short& hs)
		{
			switch (aes_mode) {
			case 0:	return true;	//	not decrypted
			case 5:	return decode_data_mode_5(tag, aes_mode, server_id, hs);
			case 7:
			case 13:
			default:
				break;
			}

			CYNG_LOG_ERROR(logger_, "AES mode "
				<< +aes_mode
				<< " is not supported - server ID: "
				<< from_server_id(server_id));

			return false;
		}

		bool data::decode_data_mode_5(boost::uuids::uuid tag, std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_long& hl)
		{
			//
			//	get AES key and decrypt content (AES CBC - mode 5)
			//
			config_db_.access([&](cyng::store::table* tbl) {

				CYNG_LOG_DEBUG(logger_, "search: "
					<< sml::from_server_id(server_id)
					<< " / "
					<< tbl->size());

				auto const rec = tbl->lookup(cyng::table::key_generator(server_id));
				if (!rec.empty()) {

					cyng::crypto::aes_128_key key;
					key = cyng::value_cast(rec["aes"], key);

					//
					//	decode payload data
					//
					if (!hl.decode(key)) {
						CYNG_LOG_WARNING(logger_, "meter "
							<< sml::from_server_id(server_id)
							<< " has invalid AES key: "
							<< cyng::io::to_hex(key.to_buffer(), ' '));
					}
				}
				else {
					CYNG_LOG_WARNING(logger_, "meter " << sml::from_server_id(server_id) << " is not configured");

					cyng::crypto::aes_128_key key;
					if (boost::algorithm::equals(sml::from_server_id(server_id), "01-e61e-29436587-bf-03") ||
						boost::algorithm::equals(sml::from_server_id(server_id), "01-e61e-13090016-3c-07"))	{
						key.key_ = { 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8, 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8 };
					}
					else if (sml::from_server_id(server_id) == "01-a815-74314504-01-02") {
						key.key_ = { 0x23, 0xA8, 0x4B, 0x07, 0xEB, 0xCB, 0xAF, 0x94, 0x88, 0x95, 0xDF, 0x0E, 0x91, 0x33, 0x52, 0x0D };
					}

					//
					//	decode payload data
					//
					if (!hl.decode(key)) {
						CYNG_LOG_WARNING(logger_, "meter "
							<< sml::from_server_id(server_id)
							<< " has invalid AES key: "
							<< cyng::io::to_hex(key.to_buffer(), ' '));
					}

					//
					//	update meter table
					//
					tbl->insert(cyng::table::key_generator(server_id)
						, cyng::table::data_generator(std::chrono::system_clock::now()
							, "+++"	//	class
							, false	//	active
							, "auto (long header)"	//	description
							, 0ull	//	status
							, cyng::buffer_t{ 0, 0 }	//	mask
							, 26000ul	//	interval
							, cyng::make_buffer({})	//	pubKey
							, key	//	 AES key 
							, ""	//	user
							, "")	//	password
						, 1	//	generation
						, tag);

				}
			}, cyng::store::write_access("mbus-devices"));

			return hl.header().verify_encryption();
		}

		bool data::decode_data_mode_5(boost::uuids::uuid tag, std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_short& hs)
		{
			//
			//	get AES key and decrypt content (AES CBC - mode 5)
			//
			config_db_.access([&](cyng::store::table* tbl) {

				CYNG_LOG_DEBUG(logger_, "search: "
					<< sml::from_server_id(server_id)
					<< " / "
					<< tbl->size());

				auto const rec = tbl->lookup(cyng::table::key_generator(server_id));
				if (!rec.empty()) {

					cyng::crypto::aes_128_key key;
					key = cyng::value_cast(rec["aes"], key);

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
				}
				else {
					CYNG_LOG_WARNING(logger_, "meter " << sml::from_server_id(server_id) << " is not configured");

					cyng::crypto::aes_128_key key;
					if (sml::from_server_id(server_id) == "01-e61e-29436587-bf-03") {
						key.key_ = { 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8, 0x51, 0x72, 0x89, 0x10, 0xE6, 0x6D, 0x83, 0xF8 };
					}
					else if (sml::from_server_id(server_id) == "01-a815-74314504-01-02") {
						key.key_ = { 0x23, 0xA8, 0x4B, 0x07, 0xEB, 0xCB, 0xAF, 0x94, 0x88, 0x95, 0xDF, 0x0E, 0x91, 0x33, 0x52, 0x0D };
					}

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

					//
					//	update meter table
					//
					tbl->insert(cyng::table::key_generator(server_id)
						, cyng::table::data_generator(std::chrono::system_clock::now()
							, "+++"	//	class
							, false	//	active
							, "auto (short header)"	//	description
							, 0ull	//	status
							, cyng::buffer_t{ 0, 0 }	//	mask
							, 26000ul	//	interval
							, cyng::make_buffer({})	//	pubKey
							, key	//	 AES key 
							, ""	//	user
							, "")	//	password
						, 1	//	generation
						, tag);

				}
			}, cyng::store::write_access("mbus-devices"));

			return hs.verify_encryption();

		}

		void data::store(cyng::context& ctx)
		{
			cyng::vector_t const frame = ctx.get_frame();
			CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

			auto const tpl = cyng::tuple_cast<
				cyng::buffer_t,		//	[0] server/meter ID
				std::uint8_t		//	[1] frame type
			>(frame);



		}

	}	//	sml
}

