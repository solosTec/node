/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#include <segw.h>
#include <lmn.h>
#include <bridge.h>
#include <cache.h>
#include <cfg_rs485.h>
#include <cfg_wmbus.h>

#include <tasks/lmn_port.h>
#include <tasks/parser_serial.h>
#include <tasks/parser_wmbus.h>
#include <tasks/parser_CP210x.h>
#include <tasks/rs485.h>
#include <tasks/iec.h>
#include <tasks/broker.h>
#include <tasks/lmn_status.h>

#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/protocol/reader.h>
#include <smf/sml/parser/obis_parser.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
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
		, decoder_(logger
			, vm_
			, std::bind(&lmn::cb_wmbus_meter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
			, std::bind(&lmn::cb_wmbus_value, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6)
		)

		, serial_mgr_(cyng::async::NO_TASK)
		, radio_port_(cyng::async::NO_TASK)
		, serial_port_(cyng::async::NO_TASK)

		, radio_distributor_(cyng::async::NO_TASK)
		, uuidgen_()
		, frq_()
	{
		cyng::register_logger(logger_, vm_);
		vm_.async_run(cyng::generate_invoke("log.msg.info", "log domain is running"));

		//
		//	callback from wired LMN
		//
		vm_.register_function("wmbus.push.frame", 8, std::bind(&lmn::wmbus_push_frame, this, std::placeholders::_1));

		//
		//	incoming data from task "parser_wmbus"
		//
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

		//
		//	define listener for configuration changes
		//
		auto l = cache_.get_db().get_listener("_Cfg"
			, std::bind(&lmn::sig_ins_cfg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
			, std::bind(&lmn::sig_del_cfg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
			, std::bind(&lmn::sig_clr_cfg, this, std::placeholders::_1, std::placeholders::_2)
			, std::bind(&lmn::sig_mod_cfg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		boost::ignore_unused(l);

	}

	void lmn::start()
	{
		//
		//	rs485 port
		//
		cfg_rs485 const rs485(cache_);
		if (rs485.is_enabled()) {
			start_lmn_wired();
		}
		else {
			CYNG_LOG_WARNING(logger_, "LMN wired RS485 is disabled (rs485:enabled)");
		}

		//
		//	wireless M-Bus port
		//
		cfg_wmbus const wmbus(cache_);
		if (wmbus.is_enabled()) {
			start_lmn_wireless(wmbus);
		}
		else {
			CYNG_LOG_WARNING(logger_, "LMN wireless M-Bus is disabled (IF_wMBUS::enabled)");
		}
	}

	void lmn::start_lmn_wired()
	{
		try {

			cfg_broker broker(cache_);		
			cfg_mbus mbus(cache_);

			//
			//	task to control SML status of wired (M-Bus)
			//
			auto const tsk_status = cyng::async::start_task_sync<lmn_status>(mux_
				, logger_
				, cache_
				, sml::STATUS_BIT_WIRED_MBUS_IF_AVAILABLE).first;

			//
			//	open port and start manager
			//	take a list of receivers
			//
			auto const sender = start_lmn_port_wired(tsk_status);
			if (sender.second) {

				//
				//	update receiver list
				//
				start_wired_mbus_receiver(
					mbus.generate_profile(), //	generating profiles can be disabled
					broker.get_server(source::WIRED_LMN));
			}
		}
		catch (boost::system::system_error const& ex) {
			CYNG_LOG_FATAL(logger_, ex.what() << ":" << ex.code());
		}
	}

	void lmn::start_lmn_wireless(cfg_wmbus const& wmbus)
	{
		try {

			//
			//	config reader
			//
			cfg_broker broker(cache_);

			//
			//	task to control SML status of wireless M-Bus
			//
			auto const tsk_status = cyng::async::start_task_sync<lmn_status>(mux_
				, logger_
				, cache_
				, sml::STATUS_BIT_WIRELESS_MBUS_IF_AVAILABLE).first;

			//
			//	the wireless sender is either a LMN port or the CP210x parser (on windows)
			//
			radio_distributor_ = start_wireless_sender(wmbus, tsk_status);

			//
			//	Update receiver list
			//
			start_wireless_mbus_receiver(wmbus.generate_profile(), broker.get_server(source::WIRELESS_LMN));

		}
		catch (boost::system::system_error const& ex) {
			CYNG_LOG_FATAL(logger_, ex.what() << ":" << ex.code());
		}
	}

	std::size_t lmn::start_wireless_sender(cfg_wmbus const& wmbus, std::size_t tsk_status)
	{
		//
		//	Data coming from CP210x (USB receiver) and have to be be unpacked.
		//
		if (wmbus.is_CP210x()) {


			//	start 
			auto const unwrapper = cyng::async::start_task_sync<parser_CP210x>(mux_
				, logger_
				, vm_);

 			if (unwrapper.second) {

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
					
				//
				//	open port for wireless M-Bus data
				//
				radio_port_ = start_lmn_port_wireless(tsk_status	//	control status bit for wireless M-Bus
					, cyng::make_buffer({ 0xA5, 0x81, 0x03, 0x17, 0x00, 0xFF, 0x00, 0x03, 0x00, 0xB3, 0x25, 0x51, 0x18, 0x10, 0x00, 0x01, 0x00, 0x01, 0xFD, 0x07, 0x32, 0x00, 0x01, 0x01, 0x02, 0x01, 0x00, 0x83, 0xC9 })).first;

				//
				//	send data from COM port to parser_CP210x
				//
				mux_.post(radio_port_, 1u, cyng::tuple_factory(unwrapper.first, unwrapper.second));

				return unwrapper.first;
			}
			else {
				CYNG_LOG_FATAL(logger_, "cannot start CP210x parser for HCI messages");
			}
		}

		//
		//	LMN port send incoming data to wmbus parser
		//
		return start_lmn_port_wireless(tsk_status, cyng::make_buffer({})).first;
	}


	void lmn::start_wireless_mbus_receiver(bool profile, server_list_t&& nodes)
	{
		//
		//	profile generation
		//
		if (profile) {
			auto const r = cyng::async::start_task_sync<parser_wmbus>(mux_
				, logger_
				, vm_
				, cache_);
			mux_.post(radio_distributor_, 1u, cyng::tuple_factory(r.first, r.second));
		}

		//
		//	broker
		//
		for (auto const node : nodes) {
			auto const r = cyng::async::start_task_sync<broker>(mux_
				, logger_
				, vm_
				, cache_
				, node.get_account()
				, node.get_pwd()
				, node.get_address()
				, node.get_port());
			//	broker are observers of table "_Readout"
			//mux_.post(radio_distributor_, 1u, cyng::tuple_factory(r.first, r.second));
		}

	}

	void lmn::start_wired_mbus_receiver(bool profile, server_list_t&& nodes)
	{

		//
		//	profile generation
		//
		if (profile) {
			//
			//	start receiver
			//
			auto const r = cyng::async::start_task_sync<parser_serial>(mux_
				, logger_
				, vm_
				, cache_);

			mux_.post(serial_port_, 1u, cyng::tuple_factory(r.first, r.second));

			//
			//	broker
			//
			for (auto const node : nodes) {
				auto const r = cyng::async::start_task_sync<broker>(mux_
					, logger_
					, vm_
					, cache_
					, node.get_account()
					, node.get_pwd()
					, node.get_address()
					, node.get_port());

				//	use signals instead of "_Readout" callbacks
				mux_.post(serial_port_, 1u, cyng::tuple_factory(r.first, r.second));
			}
		}
	}


	std::pair<std::size_t, bool> lmn::start_lmn_port_wireless(std::size_t status_receiver
		, cyng::buffer_t&& init)
	{
		cfg_wmbus cfg(cache_);

		CYNG_LOG_INFO(logger_, "LMN wireless is running on port: " << cfg.get_port());

		return cyng::async::start_task_sync<lmn_port>(mux_
			, logger_
			, cfg.get_monitor()
			, cfg.get_port()			//	port
			, cfg.get_databits()		//	[u8] databits
			, cfg.get_parity()			//	[s] parity
			, cfg.get_flow_control()	//	[s] flow_control
			, cfg.get_stopbits()		//	[s] stopbits
			, cfg.get_baud_rate()		//	[u32] speed
			, status_receiver			//	receive status change
			, std::move(init));			//	initialization message
	}

	std::pair<std::size_t, bool> lmn::start_lmn_port_wired(std::size_t status_receiver)
	{
		cfg_rs485 cfg(cache_);

		auto const parity = cfg.get_parity();
		auto const r = cyng::async::start_task_delayed<lmn_port>(mux_
			, cfg.get_delay(std::chrono::seconds(4))	//	use monitor as delay timer
			, logger_
			, cfg.get_monitor()
			, cfg.get_port()			//	port
			, cfg.get_databits()		//	[u8] databits
			, cfg.get_parity()			//	[s] parity
			, cfg.get_flow_control()	//	[s] flow_control
			, cfg.get_stopbits()		//	[s] stopbits
			, cfg.get_baud_rate()		//	[u32] speed
			, status_receiver			//	receive status change
			, cyng::make_buffer({}));	//	no initialization data

		if (r.second) {

			CYNG_LOG_INFO(logger_, "LMN wired port #" 
				<< r.first
				<< " :"
				<< cfg.get_port());

			//
			//	update SERIAL_TASK
			//
			cfg.set_lmn_task(r.first);

			serial_mgr_ = r.first;

			switch (cfg.get_protocol()) {
			case cfg_rs485::protocol::MBUS:
				CYNG_LOG_INFO(logger_, cfg.get_port() << " is managed as wired M-Bus");
				return start_rs485_mgr(r.first, cfg.get_monitor() + std::chrono::seconds(2));
			case cfg_rs485::protocol::IEC:
				CYNG_LOG_WARNING(logger_, "IEC protocol not supported on " << cfg.get_port());
				return start_iec_mgr(r.first, cfg.get_monitor() + std::chrono::seconds(2));
			case cfg_rs485::protocol::SML:
				CYNG_LOG_WARNING(logger_, "SML protocol not supported on " << cfg.get_port());
				break;
			default:
				CYNG_LOG_INFO(logger_, cfg.get_port() << " is unmanaged (raw)");
				break;
			}
		}
		else {
			CYNG_LOG_ERROR(logger_, "cannot start wired LMN port:"
				<< cfg.get_port());
		}
		return r;
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

	std::pair<std::size_t, bool> lmn::start_iec_mgr(std::size_t tsk, std::chrono::seconds delay)
	{
		serial_port_ = tsk;
		return cyng::async::start_task_delayed<iec>(mux_
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
		auto const [srv_id, manufacturer, version, medium, dev_id, frame_type, size, payload] = cyng::tuple_cast<
			cyng::buffer_t,		//	[0] server id
			cyng::buffer_t,		//	[1] manufacturer
			std::uint8_t,		//	[2] version
			std::uint8_t,		//	[3] medium
			std::uint32_t,		//	[4] device id
			std::uint8_t,		//	[5] frame_type
			std::uint8_t,		//	[6] size
			cyng::buffer_t		//	[7] payload
		>(frame);

		BOOST_ASSERT(manufacturer.size() == 2);
		auto const maker = sml::decode(manufacturer.at(0), manufacturer.at(1));

		CYNG_LOG_DEBUG(logger_, ctx.get_name() 
			<< " - server id: " 
			<< sml::from_server_id(srv_id));
		CYNG_LOG_DEBUG(logger_, ctx.get_name() 
			<< " - manufacturer: " 
			<< maker);
		
		CYNG_LOG_DEBUG(logger_, ctx.get_name() 
			<< " - version: " 
			<< +version);
		CYNG_LOG_DEBUG(logger_, ctx.get_name() 
			<< " - medium: " 
			<< +medium
			<< " - " 
			<< node::mbus::get_medium_name(medium));
		CYNG_LOG_DEBUG(logger_, ctx.get_name() 
			<< " - device id: " 
			<< std::setw(8)
			<< std::setfill('0')
			<< dev_id);
		CYNG_LOG_DEBUG(logger_, ctx.get_name() 
			<< " - frame type: 0x" 
			<< std::hex
			<< +frame_type);
		CYNG_LOG_DEBUG(logger_, ctx.get_name()
			<< " - data: "
			<< cyng::io::to_hex(payload));

		if (boost::algorithm::equals(sml::from_server_id(srv_id), "01-e61e-79426800-02-0e")) {

			//
			//	gas meter
			//
			CYNG_LOG_DEBUG(logger_, ctx.get_name()
				<< "\n\n GAS METER 01-e61e-79426800-02-0e\n\n");
		}
		
		//
		//	check block list
		//
		cfg_wmbus const wmbus(cache_);
		if (!wmbus.is_meter_blocked(dev_id)) {


			//	"max-readout-frequency"
			auto const max_frq = wmbus.get_monitor();

			//bool do_insert{ true };
			if (test_frq(max_frq, srv_id)) {

				//
				//	new readout pk
				//
				auto const pk = uuidgen_();

				//
				//	generate cb_wmbus_data
				//
				//
				//	"_Readout"
				//
				cache_.write_table("_Readout", [&](cyng::store::table* tbl) {


					if (!tbl->insert(cyng::table::key_generator(pk)
						, cyng::table::data_generator(srv_id
							, manufacturer
							, version
							, medium
							, dev_id
							, frame_type
							, size
							, payload
							, std::chrono::system_clock::now()
						)
						, 1u	//	generation
						, cache_.get_tag())) {

						CYNG_LOG_ERROR(logger_, ctx.get_name()
							<< " - write into \"_Readout\" table failed");
					}
					});


				//
				//	get AES key and decrypt content (AES CBC - mode 5)
				//
				auto const aes = cache_.get_aes_key(srv_id);

				decoder_.run(pk
					, srv_id
					, frame_type	//	frame type
					, payload
					, aes);


				//
				// update device table
				//
				if (cache_.update_device_table(srv_id
					, maker
					, 0u	//	status
					, version
					, medium
					, aes
					, ctx.tag())) {

					CYNG_LOG_INFO(logger_, "new device: "
						<< sml::from_server_id(srv_id));
				}
			}
			else {
				//	"max-readout-frequency"
				CYNG_LOG_WARNING(logger_, "device "
					<< sml::from_server_id(srv_id)
					<< " has max-readout-frequency of "
					<< max_frq.count()
					<< " seconds exceede");

			}
		}
		else {

			CYNG_LOG_WARNING(logger_, "device "
				<< sml::from_server_id(srv_id)
				<< " is blocked" );
		}
	}

	bool lmn::test_frq(std::chrono::seconds max_frq, cyng::buffer_t const& srv_id)
	{
		if (max_frq.count() != 0) {
			auto const now = std::chrono::system_clock::now();
			auto pos = frq_.find(srv_id);
			if (pos != frq_.end()) {
				auto const cmp = std::chrono::duration_cast<std::chrono::seconds>(now - pos->second);
				//	compare
				if (cmp > max_frq) {

					//	update
					pos->second = now;

					//	last entry is in range
					return true;
				}
			}
			else {
				//	insert
				frq_.emplace(srv_id, now);
			}
		}
		return true;
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

		auto const frame = ctx.get_frame();
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

	void lmn::sig_ins_cfg(cyng::store::table const* tbl
		, cyng::table::key_type const&
		, cyng::table::data_type const&
		, std::uint64_t
		, boost::uuids::uuid)
	{
		BOOST_ASSERT(boost::algorithm::equals(tbl->meta().get_name(), "_Cfg"));
	}

	void lmn::sig_del_cfg(cyng::store::table const* tbl, cyng::table::key_type const&, boost::uuids::uuid)
	{
		BOOST_ASSERT(boost::algorithm::equals(tbl->meta().get_name(), "_Cfg"));
	}

	void lmn::sig_clr_cfg(cyng::store::table const* tbl, boost::uuids::uuid)
	{
		BOOST_ASSERT(boost::algorithm::equals(tbl->meta().get_name(), "_Cfg"));
	}

	void lmn::sig_mod_cfg(cyng::store::table const* tbl
		, cyng::table::key_type const& key
		, cyng::attr_t const& attr
		, std::uint64_t gen
		, boost::uuids::uuid source)
	{
		BOOST_ASSERT(boost::algorithm::equals(tbl->meta().get_name(), "_Cfg"));
		BOOST_ASSERT(key.size() == 1);

		auto const name = cyng::value_cast(key.at(0), "");
#ifdef _DEBUG
		if (boost::algorithm::starts_with(name, "9100000000FF")) {
			CYNG_LOG_INFO(logger_, "LMN " << name << ": " << cyng::io::to_type(attr.second));
		}
#endif
		if (boost::algorithm::equals(name, build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, 1), sml::make_obis(sml::OBIS_SERIAL_DATABITS, 1) }))) {
			//	wireless
			mux_.post(radio_port_, 2u, cyng::tuple_factory(sml::OBIS_SERIAL_DATABITS.to_buffer(), attr.second));
		}
		else if (boost::algorithm::equals(name, build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, 2), sml::make_obis(sml::OBIS_SERIAL_DATABITS, 2) }))) {
			//	RS 485
			mux_.post(serial_port_, 2u, cyng::tuple_factory(sml::OBIS_SERIAL_DATABITS.to_buffer(), attr.second));
		}
		else if (boost::algorithm::equals(name, build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, 1), sml::make_obis(sml::OBIS_SERIAL_PARITY, 1) }))) {
			mux_.post(radio_port_, 3u, cyng::tuple_factory(sml::OBIS_SERIAL_PARITY.to_buffer(), attr.second));
		}
		else if (boost::algorithm::equals(name, build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, 2), sml::make_obis(sml::OBIS_SERIAL_PARITY, 2) }))) {
			mux_.post(serial_port_, 3u, cyng::tuple_factory(sml::OBIS_SERIAL_PARITY.to_buffer(), attr.second));
		}
		else if (boost::algorithm::equals(name, build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, 1), sml::make_obis(sml::OBIS_SERIAL_FLOW_CONTROL, 1) }))) {
			mux_.post(radio_port_, 3u, cyng::tuple_factory(sml::OBIS_SERIAL_FLOW_CONTROL.to_buffer(), attr.second));
		}
		else if (boost::algorithm::equals(name, build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, 2), sml::make_obis(sml::OBIS_SERIAL_FLOW_CONTROL, 2) }))) {
			mux_.post(serial_port_, 3u, cyng::tuple_factory(sml::OBIS_SERIAL_FLOW_CONTROL.to_buffer(), attr.second));
		}
		else if (boost::algorithm::equals(name, build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, 1), sml::make_obis(sml::OBIS_SERIAL_STOPBITS, 1) }))) {
			mux_.post(radio_port_, 3u, cyng::tuple_factory(sml::OBIS_SERIAL_STOPBITS.to_buffer(), attr.second));
		}
		else if (boost::algorithm::equals(name, build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, 2), sml::make_obis(sml::OBIS_SERIAL_STOPBITS, 2) }))) {
			mux_.post(serial_port_, 3u, cyng::tuple_factory(sml::OBIS_SERIAL_STOPBITS.to_buffer(), attr.second));
		}
		else if (boost::algorithm::equals(name, build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, 1), sml::make_obis(sml::OBIS_SERIAL_SPEED, 1) }))) {
			mux_.post(radio_port_, 2u, cyng::tuple_factory(sml::OBIS_SERIAL_SPEED.to_buffer(), attr.second));
		}
		else if (boost::algorithm::equals(name, build_cfg_key({ sml::OBIS_ROOT_SERIAL, sml::make_obis(sml::OBIS_ROOT_SERIAL, 2), sml::make_obis(sml::OBIS_SERIAL_SPEED, 2) }))) {
			mux_.post(serial_port_, 2u, cyng::tuple_factory(sml::OBIS_SERIAL_SPEED.to_buffer(), attr.second));
		}
		else if (boost::algorithm::equals(name, build_cfg_key({sml::OBIS_ROOT_BROKER, sml::make_obis(sml::OBIS_ROOT_BROKER, 1) }, "enabled"))) {
			auto const enable = cyng::value_cast(attr.second, true);
			if (enable) {
				CYNG_LOG_INFO(logger_, "start broker for wireless M-Bus");
			//	cfg_wmbus const wmbus(cache_);
			//	radio_port_ = start_wireless_sender(wmbus, 0);
			}
			else {
				CYNG_LOG_WARNING(logger_, "stop broker wireless M-Bus");
				mux_.stop("node::broker");
				//	mux_.stop(radio_port_);
			}
		}
		else if (boost::algorithm::equals(name, build_cfg_key({ sml::OBIS_ROOT_BROKER, sml::make_obis(sml::OBIS_ROOT_BROKER, 2) }, "enabled"))) {
			auto const enable = cyng::value_cast(attr.second, true);
			if (enable) {
				CYNG_LOG_INFO(logger_, "start broker on RS 485");
				//	start_lmn_port_wired(0);
			}
			else {
				CYNG_LOG_WARNING(logger_, "stop broker on RS 485");
				mux_.stop("node::broker");
			}
		}
	}

	void lmn::cb_wmbus_meter(cyng::buffer_t const& srv_id
		, std::uint8_t status
		, std::uint8_t aes_mode
		, cyng::crypto::aes_128_key const& aes)
	{
		auto const manufacturer = sml::get_manufacturer_code(srv_id);

		CYNG_LOG_DEBUG(logger_, "server ID   : " << sml::from_server_id(srv_id));	//	OBIS_SERIAL_NR - 00 00 60 01 00 FF
		CYNG_LOG_DEBUG(logger_, "medium      : " << mbus::get_medium_name(sml::get_medium_code(srv_id)));
		CYNG_LOG_DEBUG(logger_, "manufacturer: " << sml::decode(manufacturer));	//	OBIS_DATA_MANUFACTURER - 81 81 C7 82 03 FF
		CYNG_LOG_DEBUG(logger_, "device id   : " << sml::get_serial(srv_id));
		CYNG_LOG_DEBUG(logger_, "status      : " << +status);
		CYNG_LOG_DEBUG(logger_, "mode        : " << +aes_mode);

		//
		// update device table
		//
		if (cache_.update_device_table(srv_id
			, sml::decode(manufacturer)
			, status	//	M-Bus status
			, sml::get_version(srv_id)
			, sml::get_medium_code(srv_id)
			, aes
			, vm_.tag())) {

			CYNG_LOG_INFO(logger_, "new device: "
				<< sml::from_server_id(srv_id));
		}

	}
	void lmn::cb_wmbus_data(cyng::buffer_t const& srv_id
		, cyng::buffer_t const& data
		, std::uint8_t wmbus_status
		, boost::uuids::uuid pk)
	{
		//
		//	read data block
		//
		CYNG_LOG_DEBUG(logger_, "decoded data of "
			<< sml::from_server_id(srv_id)
			<< ": "
			<< cyng::io::to_hex(data));

		//
		//	"_Readout"
		//
		cache_.write_table("_Readout", [&](cyng::store::table* tbl) {
			tbl->insert(cyng::table::key_generator(pk)
				, cyng::table::data_generator(srv_id, std::chrono::system_clock::now(), wmbus_status)
				, 1u	//	generation
				, cache_.get_tag());
			});

	}

	void lmn::cb_wmbus_value(cyng::buffer_t const& srv_id
		, cyng::object const& val
		, std::uint8_t scaler
		, mbus::units unit
		, sml::obis code
		, boost::uuids::uuid pk)
	{
		CYNG_LOG_TRACE(logger_, "readout (wireless mBus) "
			<< sml::from_server_id(srv_id)
			<< " value: "
			<< cyng::io::to_type(val)
			<< " "
			<< mbus::get_unit_name(unit));

		cache_.write_table("_ReadoutData", [&](cyng::store::table* tbl) {

			auto const type = static_cast<std::uint32_t>(val.get_class().tag());
			auto const unit_v = static_cast<std::uint8_t>(unit);

			tbl->insert(cyng::table::key_generator(pk, code.to_buffer())
				, cyng::table::data_generator(val, type, scaler, unit_v)
				, 1u	//	generation
				, cache_.get_tag());
		});

	}

}

