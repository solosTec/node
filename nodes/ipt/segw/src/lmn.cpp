/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#include "segw.h"
#include "lmn.h"
#include "bridge.h"
#include "cache.h"
#include "tasks/lmn_port.h"
#include "tasks/parser_serial.h"
#include "tasks/parser_wmbus.h"
#include "tasks/parser_CP210x.h"

#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/protocol/reader.h>
#include <smf/sml/parser/obis_parser.h>
#include <smf/mbus/defs.h>

#include <cyng/vm/domain/log_domain.h>
#include <cyng/tuple_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/buffer_cast.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_buffer.h>
#include <cyng/io/hex_dump.hpp>
#include <cyng/async/task/task_builder.hpp>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace node
{
	lmn::lmn(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, bridge& b)
	: mux_(mux)
		, logger_(logger)
		, vm_(mux.get_io_service(), tag)
		, bridge_(b)
		, decoder_wmbus_(logger, b.cache_, vm_)
	{
		cyng::register_logger(logger_, vm_);
		vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

		//
		//	callback from wireless LMN
		//
		vm_.register_function("wmbus.push.frame", 7, std::bind(&lmn::wmbus_push_frame, this, std::placeholders::_1));

		vm_.register_function("sml.msg", 3, std::bind(&lmn::sml_msg, this, std::placeholders::_1));
		//vm.register_function("sml.eom", 2, std::bind(&lmn::sml_eom, this, std::placeholders::_1));

		vm_.register_function("sml.get.list.response", 5, std::bind(&lmn::sml_get_list_response, this, std::placeholders::_1));

	}

	void lmn::start()
	{
		auto const wired = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "enabled"), false);
		if (wired) {
			start_lmn_wired();
		}
		else {
			CYNG_LOG_WARNING(logger_, "LMN wired (IF_1107) is disabled");
		}

		auto const wireless = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "enabled"), false);
		if (wireless) {
			start_lmn_wireless();
		}
		else {
			CYNG_LOG_WARNING(logger_, "LMN wireless (M-Bus) is disabled");
		}
	}

	void lmn::start_lmn_wired()
	{
		try {

			//
			//	start receiver
			//
			auto const receiver = cyng::async::start_task_sync<parser_serial>(mux_
				, logger_
				, vm_);

			if (receiver.second) {

				auto const sender = start_lmn_port_wired(receiver.first);

			}
		}
		catch (boost::system::system_error const& ex) {
			CYNG_LOG_FATAL(logger_, ex.what() << ":" << ex.code());
		}
	}

	void lmn::start_lmn_wireless()
	{
		try {

			//
			//	start receiver
			//
			auto const receiver = cyng::async::start_task_sync<parser_wmbus>(mux_
				, logger_
				, vm_);

			if (receiver.second) {


				auto const hci = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "HCI"), std::string("none"));
				if (boost::algorithm::equals(hci, "CP210x")) {

					//
					//	LMN port send incoming data to HCI (CP210x) parser
					//	which sends data to wmbus parser.
					//
					auto const unwrapper = cyng::async::start_task_sync<parser_CP210x>(mux_
						, logger_
						, vm_
						, receiver.first);

					if (unwrapper.second) {
						auto const sender = start_lmn_port_wireless(unwrapper.first);
					}
					else {
						CYNG_LOG_FATAL(logger_, "cannot start CP210x parser for HCI messages");
					}
				}
				else {

					//
					//	LMN port send incoming data to wmbus parser
					//
					auto const sender = start_lmn_port_wireless(receiver.first);
				}
			}
			else {
				CYNG_LOG_FATAL(logger_, "cannot start receiver for wireless mbus data");
			}
		}
		catch (boost::system::system_error const& ex) {
			CYNG_LOG_FATAL(logger_, ex.what() << ":" << ex.code());
		}
	}

	std::pair<std::size_t, bool> lmn::start_lmn_port_wireless(std::size_t receiver)
	{
#if BOOST_OS_WINDOWS
		auto const port = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "port"), std::string("COM3"));
#else
		auto const port = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "port"), std::string("/dev/ttyAPP1"));
#endif
		CYNG_LOG_INFO(logger_, "LMN wireless is running on port: " << port);

		std::chrono::seconds const monitor(bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "monitor"), 30));
		auto const speed = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "speed"), 57600);

		return cyng::async::start_task_delayed<lmn_port>(mux_
			, std::chrono::seconds(1)
			, logger_
			, monitor
			, port		//	port
			, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "databits"), 8u)		//	[u8] databits
			, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "parity"), "none")	//	[s] parity
			, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "flow_control"), "none")	//	[s] flow_control
			, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "stopbits"), "one")	//	[s] stopbits
			, static_cast<std::uint32_t>(speed)		//	[u32] speed
			, receiver);
	}

	std::pair<std::size_t, bool> lmn::start_lmn_port_wired(std::size_t receiver)
	{
#if BOOST_OS_WINDOWS
		auto const port = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "port"), std::string("COM1"));
#else
		auto const port = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "port"), std::string("/dev/ttyAPP0"));
#endif
		CYNG_LOG_INFO(logger_, "LMN wired is running on port: " << port);

		std::chrono::seconds const monitor(bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "monitor"), 30));
		auto const speed = bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_W_MBUS_PROTOCOL }, "speed"), 115200);

		return cyng::async::start_task_delayed<lmn_port>(mux_
			, std::chrono::seconds(1)
			, logger_
			, monitor
			, port		//	port
			, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "databits"), 8u)		//	[u8] databits
			, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "parity"), "none")	//	[s] parity
			, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "flow_control"), "none")	//	[s] flow_control
			, bridge_.cache_.get_cfg(build_cfg_key({ sml::OBIS_CODE_IF_1107 }, "stopbits"), "one")	//	[s] stopbits
			, static_cast<std::uint32_t>(speed)		//	[u32] speed
			, receiver);

	}


	void lmn::wmbus_push_frame(cyng::context& ctx)
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

		CYNG_LOG_DEBUG(logger_, ctx.get_name() 
			<< " - server id: " 
			<< sml::from_server_id(server_id));
		CYNG_LOG_DEBUG(logger_, ctx.get_name() 
			<< " - manufacturer: " 
			<< std::get<1>(tpl));
		CYNG_LOG_DEBUG(logger_, ctx.get_name() 
			<< " - version: " 
			<< +std::get<2>(tpl));
		CYNG_LOG_DEBUG(logger_, ctx.get_name() 
			<< " - media: " 
			<< +std::get<3>(tpl) 
			<< " - " 
			<< node::mbus::get_medium_name(std::get<3>(tpl)));
		CYNG_LOG_DEBUG(logger_, ctx.get_name() 
			<< " - device id: " 
			<< std::setw(8)
			<< std::setfill('0')
			<< std::get<4>(tpl));
		CYNG_LOG_DEBUG(logger_, ctx.get_name() 
			<< " - frame type: 0x" 
			<< std::hex
			<< +std::get<5>(tpl));
		CYNG_LOG_DEBUG(logger_, ctx.get_name()
			<< " - data: "
			<< cyng::io::to_hex(std::get<6>(tpl)));

		if (boost::algorithm::equals(sml::from_server_id(server_id), "01-e61e-79426800-02-0e")) {

			//
			//	gas meter
			//
			CYNG_LOG_DEBUG(logger_, ctx.get_name()
				<< "\n\n GAS METER 01-e61e-79426800-02-0e\n\n");
		}
		
		//
		//	decoding depends on frame type and AES key from
		//	device table
		//
		switch (std::get<5>(tpl)) {
		case node::mbus::FIELD_CI_HEADER_LONG:	//	0x72
			//	e.g. HYD
			decoder_wmbus_.read_frame_header_long(server_id, payload);
			break;
		case node::mbus::FIELD_CI_HEADER_SHORT:	//	0x7A
			decoder_wmbus_.read_frame_header_short(server_id, payload);
			break;
		case node::mbus::FIELD_CI_RES_SHORT_SML:	//	0x7F
			//	e.g. ED300L
			decoder_wmbus_.read_frame_header_short_sml(server_id, payload);
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
		break;
		}

		//
		// update device table
		//
		if (bridge_.cache_.update_device_table(std::get<0>(tpl)	
			, std::get<1>(tpl)
			, 0u	//	status
			, std::get<2>(tpl)
			, std::get<3>(tpl)
			, cyng::crypto::aes_128_key{}
			, ctx.tag())) {

			CYNG_LOG_INFO(logger_, "new device: "
				<< sml::from_server_id(server_id));
		}
	}

	void lmn::sml_msg(cyng::context& ctx)
	{
		//	example:
		//	[]
		//
		const cyng::vector_t frame = ctx.get_frame();
		//CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//
		//	get message body
		//
		auto const msg = cyng::to_tuple(frame.at(0));
		auto const pk = cyng::value_cast(frame.at(2), boost::uuids::nil_uuid());

		//
		//	add generated instruction vector
		//	"sml.get.list.response"
		//
		ctx.queue(sml::reader::read(msg, pk));
	}

	void lmn::sml_eom(cyng::context& ctx)
	{
		//	example:
		//	[]
		//
		//	* CRC
		//	* message counter
		//
		const cyng::vector_t frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

	}

	void lmn::sml_get_list_response(cyng::context& ctx)
	{
		//	
		//	example:
		//	[	.,
		//		01A815743145040102,
		//		990000000003,
		//		%(("abortOnError":0),("actGatewayTime":null),("actSensorTime":null),("code":0701),("crc16":ba45),("groupNo":0),("listSignature":null),("values":
		//		%(("0100010800FF":%(("raw":14521),("scaler":-1),("status":00020240),("unit":1e),("valTime":null),("value":1452.1))),
		//		("0100010801FF":%(("raw":0),("scaler":-1),("status":null),("unit":1e),("valTime":null),("value":0))),
		//		("0100010802FF":%(("raw":14521),("scaler":-1),("status":null),("unit":1e),("valTime":null),("value":1452.1))),
		//		("0100020800FF":%(("raw":561139),("scaler":-1),("status":00020240),("unit":1e),("valTime":null),("value":56113.9))),
		//		("0100020801FF":%(("raw":0),("scaler":-1),("status":null),("unit":1e),("valTime":null),("value":0))),
		//		("0100020802FF":%(("raw":561139),("scaler":-1),("status":null),("unit":1e),("valTime":null),("value":56113.9))),
		//		("0100100700FF":%(("raw":0),("scaler":-1),("status":null),("unit":1b),("valTime":null),("value":0))))))]
		//
		//	* [string] trx
		//	* [buffer] server id
		//	* [buffer] OBIS code
		//	* [obj] values
		//	* [uuid] pk
		//	

		cyng::vector_t const frame = ctx.get_frame();
		//CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//
		//	get server/meter id
		//
		auto const srv_id = cyng::to_buffer(frame.at(1));
		CYNG_LOG_TRACE(logger_, ctx.get_name() << " - " << sml::from_server_id(srv_id));

		//
		//	pk is the reference into the "_Readout" table
		//
		auto const pk = cyng::value_cast(frame.at(4), boost::uuids::nil_uuid());
		auto const params = cyng::to_param_map(frame.at(3));
#ifdef _DEBUG
		//for (auto const& val : params) {

		//	CYNG_LOG_DEBUG(logger_, val.first << " = " << cyng::io::to_str(val.second));

		//}
#endif
		auto pos = params.find("values");
		if (pos != params.end()) {

			auto const values = cyng::to_param_map(pos->second);
			bridge_.cache_.write_table("_ReadoutData", [&](cyng::store::table* tbl) {

				for (auto const& val : values) {

					//CYNG_LOG_DEBUG(logger_, val.first << " = " << cyng::io::to_str(val.second));

					auto const code = sml::parse_obis(val.first);
					if (code.second) {

						auto const data = cyng::to_param_map(val.second);
						auto const raw = data.at("raw");
						auto type = raw.get_class().tag();
						CYNG_LOG_DEBUG(logger_, val.first << " = " << cyng::io::to_str(data.at("value")));

						tbl->insert(cyng::table::key_generator(pk, code.first.to_buffer())
							, cyng::table::data_generator(raw, type, data.at("scaler"), data.at("unit"))
							, 1u	//	generation
							, bridge_.cache_.get_tag());
					}
					else {
						//	invalid OBIS code
					}
				}
			});

		}
		else {
			CYNG_LOG_WARNING(logger_, "sml_get_list_response(" << sml::from_server_id(srv_id) << ") has no values");
		}

	}
}

