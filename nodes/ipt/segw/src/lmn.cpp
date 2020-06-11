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
#include "cfg_rs485.h"
#include "cfg_wmbus.h"

#include "tasks/lmn_port.h"
#include "tasks/parser_serial.h"
#include "tasks/parser_wmbus.h"
#include "tasks/parser_CP210x.h"
#include "tasks/rs485.h"

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
	lmn::lmn(cyng::async::mux& m
		, cyng::logging::log_ptr logger
		, cache& cfg
		, boost::uuids::uuid tag)
	: logger_(logger)
		, mux_(m)
		, vm_(m.get_io_service(), tag)
		, cache_(cfg)
		, decoder_wmbus_(logger, cfg, vm_)

		, serial_parser_(cyng::async::NO_TASK)
		, serial_mgr_(cyng::async::NO_TASK)
		, serial_port_(cyng::async::NO_TASK)

		, radio_parser_(cyng::async::NO_TASK)
		, radio_port_(cyng::async::NO_TASK)
	{
		cyng::register_logger(logger_, vm_);
		vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

		//
		//	callback from wireless LMN
		//
		vm_.register_function("wmbus.push.frame", 7, std::bind(&lmn::wmbus_push_frame, this, std::placeholders::_1));

		vm_.register_function("sml.msg", 3, std::bind(&lmn::sml_msg, this, std::placeholders::_1));
		//vm.register_function("sml.eom", 2, std::bind(&lmn::sml_eom, this, std::placeholders::_1));

		//
		//	Comes from SML reader
		//	This is the only message that wmBus can generate
		//
		vm_.register_function("sml.get.list.response", 5, std::bind(&lmn::sml_get_list_response, this, std::placeholders::_1));

		//
		//	callback from wired LMN
		//
		vm_.register_function("mbus.ack", 0, std::bind(&lmn::mbus_ack, this, std::placeholders::_1));
		vm_.register_function("mbus.short.frame", 5, std::bind(&lmn::mbus_frame_short, this, std::placeholders::_1));
		vm_.register_function("mbus.ctrl.frame", 6, std::bind(&lmn::mbus_frame_ctrl, this, std::placeholders::_1));
		vm_.register_function("mbus.long.frame", 7, std::bind(&lmn::mbus_frame_long, this, std::placeholders::_1));

	}

	void lmn::start()
	{
		auto const wired = cache_.get_cfg(build_cfg_key({ "rs485", "enabled" }), false);
		if (wired) {
			start_lmn_wired();
		}
		else {
			CYNG_LOG_WARNING(logger_, "LMN wired (RS485) is disabled");
		}

		auto const wireless = cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "enabled"), false);	
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
				, vm_
				, cache_);

			if (receiver.second) {

				serial_parser_ = receiver.first;

				//
				//	open port and start manager
				//
				auto const sender = start_lmn_port_wired(receiver.first);
				if (sender.second) {
					serial_mgr_ = sender.first;
				}
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
				, vm_
				, cache_);

			if (receiver.second) {

				radio_parser_ = receiver.first;

				auto const hci = cache_.get_cfg(build_cfg_key({ sml::OBIS_IF_wMBUS }, "HCI"), std::string("none"));
				if (boost::algorithm::equals(hci, "CP210x")) {

					//
					//	LMN port send incoming data to HCI (CP210x) parser
					//	which sends data to wmbus parser.
					//	CP210x hst to bi initialized with:
					//	
					//	Device Mode: Other (0)
					//	Link Mode: T1 (3)
					//	Ctrl Field: 0x00
					//	Man ID: 0x25B3
					//	Device ID: 0x00101851
					//	Version: 0x01
					//	Device Type: 0x00
					//	Radio Channel: 1 (1..8)
					//	Radio Power Level: 13db (7) [0=-8dB,1=-5dB,2=-2dB,3=1dB,4=4dB,5=7dB,6=10dB,7=13dB]
					//	Rx Window (ms): 50 (0x32)
					//	Power Saving Mode: 0
					//	RSSI Attachment: 1
					//	Rx Timestamp: 1
					//	LED Control: 2
					//	RTC: 1
					//	Store NVM: 0
					//	
					//	[A5 81 03 17 00 FF] 00 03 00 [B3 25] [51 18 10 00] 01
					//	00 01 FD 07 32 00 01 01 02 01 00 [83 C9]
					//
					//	Fletcher Checksum (https://www.silabs.com/documents/public/application-notes/AN978-cp210x-usb-to-uart-api-specification.pdf)

					auto init = cyng::make_buffer({ 0xA5, 0x81, 0x03, 0x17, 0x00, 0xFF, 0x00, 0x03, 0x00, 0xB3, 0x25, 0x51, 0x18, 0x10, 0x00, 0x01, 0x00, 0x01, 0xFD, 0x07, 0x32, 0x00, 0x01, 0x01, 0x02, 0x01, 0x00, 0x83, 0xC9 });

					//
					auto const unwrapper = cyng::async::start_task_sync<parser_CP210x>(mux_
						, logger_
						, vm_
						, radio_parser_);

					if (unwrapper.second) {
						auto const sender = start_lmn_port_wireless(unwrapper.first
							, radio_parser_
							, std::move(init));
						if (sender.second) {
							radio_port_ = sender.first;
						}
					}
					else {
						CYNG_LOG_FATAL(logger_, "cannot start CP210x parser for HCI messages");
					}
				}
				else {

					BOOST_ASSERT(boost::algorithm::equals("none", hci));
					//
					//	LMN port send incoming data to wmbus parser
					//
					auto const sender = start_lmn_port_wireless(receiver.first, receiver.first, cyng::make_buffer({}));
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

	std::pair<std::size_t, bool> lmn::start_lmn_port_wireless(std::size_t receiver_data
		, std::size_t receiver_status
		, cyng::buffer_t&& init)
	{
		cfg_wmbus cfg(cache_);

		CYNG_LOG_INFO(logger_, "LMN wireless is running on port: " << cfg.get_port());

		return cyng::async::start_task_delayed<lmn_port>(mux_
			, cfg.get_monitor()	//	use monitor as delay timer
			, logger_
			, cfg.get_monitor()
			, cfg.get_port()			//	port
			, cfg.get_databits()		//	[u8] databits
			, cfg.get_parity()			//	[s] parity
			, cfg.get_flow_control()	//	[s] flow_control
			, cfg.get_stopbits()		//	[s] stopbits
			, cfg.get_baud_rate()		//	[u32] speed
			, receiver_data				//	optional unwrapper
			, receiver_status			//	receive status change
			, std::move(init));					//	initialization message
	}

	std::pair<std::size_t, bool> lmn::start_lmn_port_wired(std::size_t receiver)
	{
		cfg_rs485 cfg(cache_);

		CYNG_LOG_INFO(logger_, "LMN wired port: " << cfg.get_port());

		auto const parity = cfg.get_parity();
		auto const r = cyng::async::start_task_delayed<lmn_port>(mux_
			, cfg.get_monitor()	//	use monitor as delay timer
			, logger_
			, cfg.get_monitor()
			, cfg.get_port()			//	port
			, cfg.get_databits()		//	[u8] databits
			, cfg.get_parity()			//	[s] parity
			, cfg.get_flow_control()	//	[s] flow_control
			, cfg.get_stopbits()		//	[s] stopbits
			, cfg.get_baud_rate()		//	[u32] speed
			, receiver					//	receive data
			, receiver
			, cyng::make_buffer({}));				//	receive status change

		return (r.second)
			? start_rs485_mgr(r.first, cfg.get_monitor() + std::chrono::seconds(2))
			: r
			;
	}

	std::pair<std::size_t, bool> lmn::start_rs485_mgr(std::size_t tsk, std::chrono::seconds delay)
	{
		serial_port_ = tsk;
		return cyng::async::start_task_delayed<rs485>(mux_
			, delay
			, logger_
			, vm_
			, cache_
			, tsk);
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
		if (cache_.update_device_table(std::get<0>(tpl)	
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

	void lmn::mbus_ack(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//
		//	forward ACK to mbus manager
		//
		mux_.post(serial_mgr_, 0, cyng::tuple_factory());
	}

	void lmn::mbus_frame_short(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));
		mux_.post(serial_mgr_, 1, cyng::to_tuple(frame));

	}
	void lmn::mbus_frame_ctrl(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));
		mux_.post(serial_mgr_, 3, cyng::to_tuple(frame));

	}
	void lmn::mbus_frame_long(cyng::context& ctx)
	{
		auto const frame = ctx.get_frame();
		CYNG_LOG_DEBUG(logger_, ctx.get_name() << " - " << cyng::io::to_str(frame));

		//	[77604552-8a5d-4575-a09c-08fb8c7c3fdf,false,8,9,72,96,55010019E61E3C07 25 000000 0C78550100190C1326000000]
		auto const tpl = cyng::tuple_cast<
			boost::uuids::uuid,	//	[0] tag
			bool,				//	[1] OK
			std::uint8_t,		//	[2] C-field
			std::uint8_t,		//	[3] A-field
			std::uint8_t,		//	[4] CI-field
			std::uint8_t,		//	[5] checksum
			cyng::buffer_t		//	[6] payload
		>(frame);

		
		mux_.post(serial_mgr_, 2, cyng::to_tuple(frame));

	}


	void lmn::sml_msg(cyng::context& ctx)
	{
		//	example:
		//	[]
		//
		auto const frame = ctx.get_frame();
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
			cache_.write_table("_ReadoutData", [&](cyng::store::table* tbl) {

				for (auto const& val : values) {

					//CYNG_LOG_DEBUG(logger_, val.first << " = " << cyng::io::to_str(val.second));

					auto const code = sml::parse_obis(val.first);
					if (code.second) {

						auto const data = cyng::to_param_map(val.second);
						auto const raw = data.at("raw");
						auto type = static_cast<std::uint32_t>(raw.get_class().tag());
						CYNG_LOG_DEBUG(logger_, val.first
							<< " = " 
							<< cyng::io::to_str(data.at("value"))
							<< ':'
							<< raw.get_class().type_name());

						tbl->insert(cyng::table::key_generator(pk, code.first.to_buffer())
							, cyng::table::data_generator(cyng::io::to_str(raw), type, data.at("scaler"), data.at("unit"))
							, 1u	//	generation
							, cache_.get_tag());
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

