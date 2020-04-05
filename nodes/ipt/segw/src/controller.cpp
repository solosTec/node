/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "controller.h"
#include "storage.h"
#include "cache.h"
#include "bridge.h"
#include "lmn.h"
#include "server/server.h"
#include "tasks/network.h"
#include <NODE_project_info.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <smf/mbus/defs.h>

#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>
#include <cyng/value_cast.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/vector_cast.hpp>
#include <cyng/sys/mac.h>
#include <cyng/rnd.h>
#include <cyng/async/mux.h>
#include <cyng/async/task/task_builder.hpp>

#include <boost/core/ignore_unused.hpp>

namespace node
{
	std::pair<std::size_t, bool> join_network(cyng::async::mux&
		, cyng::logging::log_ptr logger
		, bridge& br
		, std::string account
		, std::string pwd
		, bool accept_all
		, boost::uuids::uuid tag);

	controller::controller(unsigned int index
		, unsigned int pool_size
		, std::string const& json_path
		, std::string node_name)
	: ctl(index
		, pool_size
		, json_path
		, node_name)
	{}

	cyng::vector_t controller::create_config(std::fstream& fout, boost::filesystem::path&& tmp, boost::filesystem::path&& cwd) const
	{

		//
		//	random uint32
		//	reconnect to master on different times
		//
		cyng::crypto::rnd_num<int> rnd_monitor(10, 60);

		//
		//	generate a random serial number with a length of
		//	8 characters
		//
		auto rnd_sn = cyng::crypto::make_rnd_num();
		std::stringstream sn_ss(rnd_sn(8));
		std::uint32_t sn{ 0 };
		sn_ss >> sn;
			
		//
		//	get adapter data
		//
		auto macs = cyng::sys::retrieve_mac48();
		if (macs.empty())
		{
			macs.push_back(cyng::generate_random_mac48());
		}
		
		//
		//	get hostname
		//
		auto const host = boost::asio::ip::host_name();

		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", uidgen_())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("log-pushdata", false)	//	log file for each channel
#if BOOST_OS_WINDOWS
				, cyng::param_factory("gpio-enabled", false)
#else
				, cyng::param_factory("gpio-enabled", true)
#endif
				, cyng::param_factory("gpio-path", "/sys/class/gpio")	//	accept only the specified MAC id
				, cyng::param_factory("gpio-list", cyng::vector_factory({46, 47, 50, 53}))
				, cyng::param_factory(sml::OBIS_OBISLOG_INTERVAL.to_str(), 15)	//	cycle time in minutes OBISLOG_INTERVAL
				, cyng::param_factory("readout-interval", 121)	//	cycle time in seconds
				, cyng::param_factory(sml::OBIS_STORAGE_TIME_SHIFT.to_str(), 0)	//	shifting storage time in seconds (affects all meters)

				, cyng::param_factory("SSL", cyng::tuple_factory(
					cyng::param_factory("cert", "client.crt"),
					cyng::param_factory("dh", "dh2048.pem")
				))

				, cyng::param_factory("DB", cyng::tuple_factory(
					cyng::param_factory("type", "SQLite"),
#if defined(NODE_CROSS_COMPILE) && defined(BOOST_OS_LINUX)
					cyng::param_factory("file-name", "/usr/local/etc/smf/segw.database"),
#else
					cyng::param_factory("file-name", (cwd / "segw.database").string()),
#endif
					cyng::param_factory("busy-timeout", 12),		//	seconds
					cyng::param_factory("watchdog", 30),		//	for database connection
					cyng::param_factory("pool-size", 1)		//	no pooling for SQLite
				))

				//	on this address the gateway acts as a server
				//	configuration interface
				, cyng::param_factory("server", cyng::tuple_factory(
					cyng::param_factory("address", "0.0.0.0"),
					cyng::param_factory("service", "7259"),
					cyng::param_factory("discover", "5798"),	//	UDP
					cyng::param_factory("account", "operator"),
					cyng::param_factory("pwd", "operator"),
					cyng::param_factory("accept-all-ids", false)	//	accept only the specified MAC id
				))

				//	hardware
				, cyng::param_factory("hardware", cyng::tuple_factory(
					cyng::param_factory("manufacturer", "solosTec"),	//	manufacturer (81 81 C7 82 03 FF - OBIS_DATA_MANUFACTURER)
					cyng::param_factory("model", "virtual.gateway"),	//	Typenschlüssel (81 81 C7 82 09 FF --> 81 81 C7 82 0A 01)
					cyng::param_factory("serial", sn),	//	Seriennummer (81 81 C7 82 09 FF --> 81 81 C7 82 0A 02)
					cyng::param_factory("class", "129-129:199.130.83*255"),	//	device class (81 81 C7 82 02 FF - OBIS_DEVICE_CLASS) "2D 2D 2D"
					//	configure server ID (MAC address)
					cyng::param_factory("mac", macs.at(0))	//	take first available MAC to build a server id (05 xx xx ..., 81 81 C7 82 04 FF - OBIS_SERVER_ID)
				))

				//	wireless M-Bus adapter
				, cyng::param_factory("adapter", cyng::tuple_factory(
					cyng::param_factory(sml::OBIS_W_MBUS_ADAPTER_MANUFACTURER.to_str(), "RC1180-MBUS3"),	//	manufacturer (81 06 00 00 01 00)
					cyng::param_factory(sml::OBIS_W_MBUS_ADAPTER_ID.to_str(), "A8 15 17 45 89 03 01 31"),	//	adapter ID (81 06 00 00 03 00)
					cyng::param_factory(sml::OBIS_W_MBUS_FIRMWARE.to_str(), "3.08"),	//	firmware (81 06 00 02 00 00)
					cyng::param_factory(sml::OBIS_W_MBUS_HARDWARE.to_str(), "2.00")	//	hardware (81 06 00 02 03 FF)
				))

				//	wireless M-Bus
				//	stty -F /dev/ttyAPP0 raw
				//	stty -F /dev/ttyAPP0  -echo -echoe -echok
				//	stty -F /dev/ttyAPP0 115200 
				//	cat /dev/ttyAPP0 | hexdump 
				, cyng::param_factory("wireless-LMN", cyng::tuple_factory(
					cyng::param_factory("monitor", rnd_monitor()),	//	seconds
#if BOOST_OS_WINDOWS
					//	iM871A
					cyng::param_factory("enabled", false),
					cyng::param_factory("port", "COM4"),	//	USB serial port
					//	if port number is greater than 9 the following syntax is required: "\\\\.\\COM12"
					cyng::param_factory("HCI", "CP210x"),	//	iM871A mbus-USB converter
					cyng::param_factory("databits", 8),
					cyng::param_factory("parity", "none"),	//	none, odd, even
					cyng::param_factory("flow-control", "none"),	//	none, software, hardware
					cyng::param_factory("stopbits", "one"),	//	one, onepointfive, two
					cyng::param_factory("speed", 57600),
#else
					cyng::param_factory("enabled", true),
					cyng::param_factory("port", "/dev/ttyAPP0"),
					cyng::param_factory("HCI", "none"),	
					cyng::param_factory("databits", 8),
					cyng::param_factory("parity", "none"),	//	none, odd, even
					cyng::param_factory("flow-control", "none"),	//	none, software, hardware
					cyng::param_factory("stopbits", "one"),	//	one, onepointfive, two
					cyng::param_factory("speed", 115200),
#endif

					cyng::param_factory(sml::OBIS_W_MBUS_PROTOCOL.to_str(), static_cast<std::uint8_t>(mbus::MODE_S)),	//	0 = T-Mode, 1 = S-Mode, 2 = S/T Automatic
					cyng::param_factory(sml::OBIS_W_MBUS_MODE_S.to_str(), 30),	//	seconds
					cyng::param_factory(sml::OBIS_W_MBUS_MODE_T.to_str(), 20),	//	seconds
					cyng::param_factory(sml::OBIS_W_MBUS_REBOOT.to_str(), 86400),	//	0 = no reboot (seconds)
					cyng::param_factory(sml::OBIS_W_MBUS_POWER.to_str(), static_cast<std::uint8_t>(mbus::STRONG)),	//	low, basic, average, strong (unused)
					cyng::param_factory(sml::OBIS_W_MBUS_INSTALL_MODE.to_str(), true),	//	install mode

					cyng::param_factory("transparent-mode", false),
					cyng::param_factory("transparent-port", 12001),
					//	if task <readout> receives data and there is no data collector/mirror defined, create one
					cyng::param_factory("autogen-data-collector", true)
					))

				, cyng::param_factory("wired-LMN", cyng::tuple_factory(
					cyng::param_factory("monitor", rnd_monitor()),	//	seconds
#if BOOST_OS_WINDOWS
					cyng::param_factory("enabled", false),
					cyng::param_factory("port", "COM6"),
					cyng::param_factory("parity", "even"),	//	none, odd, even
#else
					cyng::param_factory("enabled", true),
					cyng::param_factory("port", "/dev/ttyAPP1"),
					cyng::param_factory("parity", "none"),	//	none, odd, even
#endif
					//	8N1
					cyng::param_factory("databits", 8),
					cyng::param_factory("flow-control", "none"),	//	none, software, hardware
					cyng::param_factory("stopbits", "one"),	//	one, onepointfive, two
					cyng::param_factory("speed", 2400),		//	initial
					cyng::param_factory("transparent-mode", false),
					cyng::param_factory("transparent-port", 12002)
				))

				, cyng::param_factory("if-1107", cyng::tuple_factory(
					//	IEC 62056-21
					cyng::param_factory(sml::OBIS_IF_1107_ACTIVE.to_str(), true),	//	active
					cyng::param_factory(sml::OBIS_IF_1107_LOOP_TIME.to_str(), 60),	//	loop timeout in seconds
					cyng::param_factory(sml::OBIS_IF_1107_RETRIES.to_str(), 3),	//	retries
					cyng::param_factory(sml::OBIS_IF_1107_MIN_TIMEOUT.to_str(), 200),	//	min. timeout (milliseconds)
					cyng::param_factory(sml::OBIS_IF_1107_MAX_TIMEOUT.to_str(), 5000),	//	max. timeout (milliseconds)
					cyng::param_factory(sml::OBIS_IF_1107_MAX_DATA_RATE.to_str(), 10240),	//	max. databytes
					cyng::param_factory(sml::OBIS_IF_1107_RS485.to_str(), true),	//	 true = RS485, false = RS232
					cyng::param_factory(sml::OBIS_IF_1107_PROTOCOL_MODE.to_str(), 2),	//	protocol mode 0 == A, 1 == B, 2 == C (A...E)
					cyng::param_factory(sml::OBIS_IF_1107_AUTO_ACTIVATION.to_str(), true),	//	auto activation
					cyng::param_factory(sml::OBIS_IF_1107_TIME_GRID.to_str(), 900),	//	900 sec = 15 min
					cyng::param_factory(sml::OBIS_IF_1107_TIME_SYNC.to_str(), 14400),	//	14400 sec = 4 h
					cyng::param_factory(sml::OBIS_IF_1107_MAX_VARIATION.to_str(), 9)	//	max. variation in seconds

				))
				, cyng::param_factory("mbus", cyng::tuple_factory(
					cyng::param_factory(sml::OBIS_CLASS_MBUS_RO_INTERVAL.to_str(), 3600),	//	readout interval in seconds
					cyng::param_factory(sml::OBIS_CLASS_MBUS_SEARCH_INTERVAL.to_str(), 0),	//	search interval in seconds
					cyng::param_factory(sml::OBIS_CLASS_MBUS_SEARCH_DEVICE.to_str(), true),	//	search device now and by restart
					cyng::param_factory(sml::OBIS_CLASS_MBUS_AUTO_ACTICATE.to_str(), false),	//	automatic activation of meters 
					cyng::param_factory(sml::OBIS_CLASS_MBUS_BITRATE.to_str(), 82)	//	used baud rates(bitmap)
				))
				, cyng::param_factory("ipt", cyng::vector_factory({
					cyng::tuple_factory(
#if defined(NODE_CROSS_COMPILE) && defined(BOOST_OS_LINUX)
						cyng::param_factory("host", "segw.ch"),
						cyng::param_factory("account", host),
						cyng::param_factory("service", "26862"),
#else
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("account", "segw"),
						cyng::param_factory("service", "26862"),
#endif
						cyng::param_factory("pwd", NODE_PWD),
						cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::param_factory("scrambled", true),
						cyng::param_factory("monitor", rnd_monitor())),	//	seconds
					cyng::tuple_factory(
#if defined(NODE_CROSS_COMPILE) && defined(BOOST_OS_LINUX)
						cyng::param_factory("host", "192.168.1.100"),
						cyng::param_factory("account", host),
						cyng::param_factory("service", "26862"),
#else
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "26863"),
						cyng::param_factory("account", "segw"),
#endif
						cyng::param_factory("pwd", NODE_PWD),
						cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::param_factory("scrambled", false),
						cyng::param_factory("monitor", rnd_monitor()))
					}))

				, cyng::param_factory("ipt-param", cyng::tuple_factory(
					cyng::param_factory(sml::OBIS_TCP_WAIT_TO_RECONNECT.to_str(), 1u),	//	minutes
					cyng::param_factory(sml::OBIS_TCP_CONNECT_RETRIES.to_str(), 3u),
					cyng::param_factory(sml::make_obis(0x00, 0x80, 0x80, 0x00, 0x03, 0x01).to_str(), 0u)
				))

				//	built-in meter
				, cyng::param_factory("virtual-meter", cyng::tuple_factory(
					cyng::param_factory("enabled", false),
					cyng::param_factory("active", true),
					cyng::param_factory("server", "01-d81c-10000001-3c-02"),	//	1CD8
					cyng::param_factory("interval", 26000)
				))
			)
		});
	}

	bool controller::start(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cyng::reader<cyng::object> const& cfg
		, boost::uuids::uuid tag)
	{
		//
		//  control push data logging
		//
		auto const log_pushdata = cyng::value_cast(cfg.get("log-pushdata"), false);
		boost::ignore_unused(log_pushdata);
		
		//
		//	global data cache
		//
		cyng::store::db config_db;

		//
		//	create tables 
		//
		init_cache(config_db);

		//
		//	setup cache manager
		//	and initialize _Cfg table
		//
		cache cm(config_db, tag);

		//
		//	setup storage manager
		//
		auto con_type = cyng::db::get_connection_type(cyng::value_cast<std::string>(cfg["DB"].get("type"), "SQLite"));
		storage store(mux.get_io_service(), con_type);

		//
		//	get database configuration and connect
		//
		cyng::tuple_t tpl;
		tpl = cyng::value_cast(cfg.get("DB"), tpl);
		auto const db_cfg = cyng::to_param_map(tpl);

		for (auto const& p : db_cfg) {
			CYNG_LOG_TRACE(logger, "db - "
				<< p.first
				<< " = "
				<< cyng::io::to_str(p.second));

		}
		if (!store.start(db_cfg)) {

			CYNG_LOG_FATAL(logger, "cannot start database connection pool");

			//
			//	shutdown
			//
			return true;
		}

		//
		//	setup bridge
		//
		bridge& br = bridge::get_instance(logger, mux, cm, store);

		//
		//	read login credentials
		//
		auto const account = cm.get_cfg<std::string>("server:account", "");
		auto const pwd = cm.get_cfg<std::string>("server:pwd", "");

		//
		//	"accept-all-ids" will never change during the session lifetime.
		//	Changes require a reboot.
		//
		auto const accept_all = cm.get_cfg("server:accept-all-ids", false);

		//
		//	connect to ipt master
		//
		auto res_ret = join_network(mux
			, logger
			, br
			, account
			, pwd
			, accept_all
			, tag);

		if (res_ret.second) {

			//
			//	create server
			//
			server srv(mux
				, logger
				, cm
				, store
				, account
				, pwd
				, accept_all);

			//
			//	server runtime configuration
			//
			auto const address = cm.get_cfg<std::string>("server:address", "");
			auto const service = cm.get_cfg<std::string>("server:service", "");

			CYNG_LOG_INFO(logger, "listener address: " << address);
			CYNG_LOG_INFO(logger, "listener service: " << service);
			srv.run(address, service);

			//
			//	data I/O manager (serial and wireless data)
			//
			lmn io(mux, logger, cm, uidgen_());
			io.start();	//	open serial and wireless communication ports

			//
			//	wait for system signals
			//
			bool const shutdown = wait(logger);

			//
			//	close acceptor
			//
			CYNG_LOG_INFO(logger, "close acceptor");
			srv.close();
		
			return shutdown;
		}
		else {

			CYNG_LOG_FATAL(logger, "cannot start network task");

		}

		return true;	//	shutdown
	}

	int controller::prepare_config_db(cyng::param_map_t&& cfg)
	{
		return (init_storage(std::move(cfg)))
			? EXIT_SUCCESS
			: EXIT_FAILURE
			;
	}

	int controller::transfer_config() const
	{
		//
		//	read configuration file
		//
		auto const r = read_config_section(json_path_, config_index_);
		if (r.second) {

			//
			//	get a DOM reader
			//
			auto const dom = cyng::make_reader(r.first);

			//
			//	get database configuration and connect
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(dom.get("DB"), tpl);
			auto db_cfg = cyng::to_param_map(tpl);

			std::cout
				<< "read configuration file "
				<< json_path_
				<< " with index ["
				<< config_index_
				<< "] into database "
				<< cyng::io::to_str(db_cfg)
				<< std::endl;

			return (transfer_config_to_storage(cyng::to_param_map(tpl), dom))
				? EXIT_SUCCESS
				: EXIT_FAILURE
				;
		}
		else
		{
			std::cout
				<< "configuration file ["
				<< json_path_
				<< "] not found or index ["
				<< config_index_
				<< "] is out of range"
				<< std::endl;
		}
		return EXIT_FAILURE;
	}

	int controller::dump_profile(std::uint32_t profile) const
	{
		//
		//	read configuration file
		//
		auto const r = read_config_section(json_path_, config_index_);
		if (r.second) {

			//
			//	get a DOM reader
			//
			auto const dom = cyng::make_reader(r.first);

			//
			//	get database configuration and connect
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(dom.get("DB"), tpl);
			auto db_cfg = cyng::to_param_map(tpl);

			std::cout
				<< "read configuration file "
				<< json_path_
				<< " with index ["
				<< config_index_
				<< "] into database "
				<< cyng::io::to_str(db_cfg)
				<< std::endl;

			if (1 == profile)	return (dump_push_ops(cyng::to_param_map(tpl), dom))
				? EXIT_SUCCESS
				: EXIT_FAILURE
				;
			return (dump_profile_data(cyng::to_param_map(tpl), dom, profile))
				? EXIT_SUCCESS
				: EXIT_FAILURE
				;
		}
		else
		{
			std::cout
				<< "configuration file ["
				<< json_path_
				<< "] not found or index ["
				<< config_index_
				<< "] is out of range"
				<< std::endl;
		}
		return EXIT_FAILURE;
	}

	int controller::list_config(std::ostream& os) const
	{
		//
		//	read configuration file
		//
		auto const r = read_config_section(json_path_, config_index_);
		if (r.second) {

			//
			//	get a DOM reader
			//
			auto const dom = cyng::make_reader(r.first);

			//
			//	get database configuration and connect
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(dom.get("DB"), tpl);
			auto db_cfg = cyng::to_param_map(tpl);

			std::cout
				<< "read configuration file "
				<< json_path_
				<< " with index ["
				<< config_index_
				<< "] into database "
				<< cyng::io::to_str(db_cfg)
				<< std::endl;

			return (list_config_data(os, cyng::to_param_map(tpl), dom))
				? EXIT_SUCCESS
				: EXIT_FAILURE
				;
		}
		else
		{
			std::cout
				<< "configuration file ["
				<< json_path_
				<< "] not found or index ["
				<< config_index_
				<< "] is out of range"
				<< std::endl;
		}
		return EXIT_FAILURE;
	}

	int controller::clear_config() const
	{
		//
		//	read configuration file
		//
		auto const r = read_config_section(json_path_, config_index_);
		if (r.second) {

			//
			//	get a DOM reader
			//
			auto const dom = cyng::make_reader(r.first);

			//
			//	get database configuration and connect
			//
			cyng::tuple_t tpl;
			tpl = cyng::value_cast(dom.get("DB"), tpl);
			auto db_cfg = cyng::to_param_map(tpl);

			std::cout
				<< "clear configuration "
				<< json_path_
				<< " with index ["
				<< config_index_
				<< "] from database "
				<< cyng::io::to_str(db_cfg)
				<< std::endl;

			auto const count = clear_config_from_storage(cyng::to_param_map(tpl), dom);
			std::cout
				<< count
				<< " row(s) removed from table TCfg"
				<< std::endl;

			return EXIT_SUCCESS;
		}
		else
		{
			std::cout
				<< "configuration file ["
				<< json_path_
				<< "] not found"
				<< config_index_
				<< "] is out of range"
				<< std::endl;
		}
		return EXIT_FAILURE;
	}

	std::pair<std::size_t, bool> join_network(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, bridge& br
		, std::string account
		, std::string pwd
		, bool accept_all
		, boost::uuids::uuid tag)
	{
		return cyng::async::start_task_sync<ipt::network>(mux
			, logger
			, br
			, account
			, pwd
			, accept_all
			, tag);

	}

}
