/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "controller.h"
#include "server.h"
#include <NODE_project_info.h>
#include "tasks/network.h"
#include "tasks/virtual_meter.h"
#include <smf/shared/db_meta.h>
#include <smf/shared/db_cfg.h>
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/status.h>
#include <smf/sml/parser/srv_id_parser.h>
#include <smf/mbus/defs.h>

#include <cyng/async/task/task_builder.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_buffer.h>
#include <cyng/parser/mac_parser.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/json.h>
#include <cyng/value_cast.hpp>
#include <cyng/numeric_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/vector_cast.hpp>
#include <cyng/sys/mac.h>
#include <cyng/store/db.h>
#include <cyng/table/meta.hpp>
#include <cyng/rnd.h>
#include <cyng/crypto/aes.h>

#include <fstream>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/core/ignore_unused.hpp>

namespace node
{
	//
	//	forward declaration(s):
	//
	void join_network(cyng::async::mux&
		, cyng::logging::log_ptr
		, cyng::store::db&
		, boost::uuids::uuid tag
		, ipt::redundancy const& cfg_ipt
		, cyng::tuple_t const& cfg_wireless_lmn
		, cyng::tuple_t const& cfg_wired_lmn
		, cyng::tuple_t const& cfg_virtual_meter
		, std::string account
		, std::string pwd
		, bool accept_all
		, std::map<int, std::string> gpio_list);

	void init_config(cyng::logging::log_ptr logger
		, cyng::store::db&
		, boost::uuids::uuid
		, cyng::reader<cyng::object> const&);

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

		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
			, cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", uidgen_())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("log-pushdata", false)	//	log file for each channel
				, cyng::param_factory("accept-all-ids", false)	//	accept only the specified MAC id
				, cyng::param_factory("gpio-path", "/sys/class/gpio")	//	accept only the specified MAC id
				, cyng::param_factory("gpio-list", cyng::vector_factory({46, 47, 50, 53}))

				//	on this address the gateway acts as a server
				//	configuration interface
				, cyng::param_factory("server", cyng::tuple_factory(
					cyng::param_factory("address", "0.0.0.0"),
					cyng::param_factory("service", "7259"),
					cyng::param_factory("discover", "5798"),	//	UDP
					cyng::param_factory("account", "operator"),
					cyng::param_factory("pwd", "operator")
				))

				//	hardware
				, cyng::param_factory("hardware", cyng::tuple_factory(
					cyng::param_factory("manufacturer", "solosTec"),	//	manufacturer (81 81 C7 82 03 FF - OBIS_DATA_MANUFACTURER)
					cyng::param_factory("model", "virtual.gateway"),	//	Typenschlüssel (81 81 C7 82 09 FF --> 81 81 C7 82 0A 01)
					cyng::param_factory("serial", sn),	//	Seriennummer (81 81 C7 82 09 FF --> 81 81 C7 82 0A 02)
					cyng::param_factory("class", "129-129:199.130.83*255"),	//	device class (81 81 C7 82 02 FF - OBIS_DEVICE_CLASS) "2D 2D 2D"
					cyng::param_factory("mac", macs.at(0))	//	take first available MAC to build a server id (05 xx xx ..., 81 81 C7 82 04 FF - OBIS_SERVER_ID)
				))

				//	wireless M-Bus
				//	stty -F /dev/ttyAPP0 raw
				//	stty -F /dev/ttyAPP0  -echo -echoe -echok
				//	stty -F /dev/ttyAPP0 115200 
				//	cat /dev/ttyAPP0 | hexdump 
				, cyng::param_factory("wireless-LMN", cyng::tuple_factory(
#if BOOST_OS_WINDOWS
					cyng::param_factory("enabled", false),
					cyng::param_factory("port", "COM3"),	//	USB serial port
					//	if port number is greater than 9 the following syntax is required: "\\\\.\\COM12"
#else
					cyng::param_factory("enabled", true),
					cyng::param_factory("port", "/dev/ttyAPP0"),
#endif
					cyng::param_factory("databits", 8),
					cyng::param_factory("parity", "none"),	//	none, odd, even
					cyng::param_factory("flow-control", "none"),	//	none, software, hardware
					cyng::param_factory("stopbits", "one"),	//	one, onepointfive, two
					cyng::param_factory("speed", 115200),

					cyng::param_factory(sml::OBIS_W_MBUS_PROTOCOL.to_str(), static_cast<std::uint8_t>(mbus::MODE_S)),	//	0 = T-Mode, 1 = S-Mode, 2 = S/T Automatic
					cyng::param_factory(sml::OBIS_W_MBUS_MODE_S.to_str(), 30),	//	seconds
					cyng::param_factory(sml::OBIS_W_MBUS_MODE_T.to_str(), 20),	//	seconds
					cyng::param_factory(sml::OBIS_W_MBUS_REBOOT.to_str(), 0),	//	0 = no reboot
					cyng::param_factory(sml::OBIS_W_MBUS_POWER.to_str(), static_cast<std::uint8_t>(mbus::STRONG)),	//	low, basic, average, strong (unused)
					cyng::param_factory(sml::OBIS_W_MBUS_INSTALL_MODE.to_str(), true),	//	install mode

					cyng::param_factory("transparent-mode", false),
					cyng::param_factory("transparent-port", 12001)
				))

				, cyng::param_factory("wired-LMN", cyng::tuple_factory(
#if BOOST_OS_WINDOWS
					cyng::param_factory("enabled", false),
					cyng::param_factory("port", "COM1"),
#else
					cyng::param_factory("enabled", true),
					cyng::param_factory("port", "/dev/ttyAPP1"),
#endif
					cyng::param_factory("databits", 8),
					cyng::param_factory("parity", "none"),	//	none, odd, even
					cyng::param_factory("flow-control", "none"),	//	none, software, hardware
					cyng::param_factory("stopbits", "one"),	//	one, onepointfive, two
					cyng::param_factory("speed", 921600),
					cyng::param_factory("transparent-mode", false),
					cyng::param_factory("transparent-port", 12002)
				))

				, cyng::param_factory("if-1107", cyng::tuple_factory(
#ifdef _DEBUG
					cyng::param_factory(sml::OBIS_IF_1107_ACTIVE.to_str() + "-descr", "OBIS_IF_1107_ACTIVE"),	//	active
					cyng::param_factory(sml::OBIS_IF_1107_LOOP_TIME.to_str() + "-descr", "OBIS_IF_1107_LOOP_TIME"),	//	loop timeout in seconds
					cyng::param_factory(sml::OBIS_IF_1107_RETRIES.to_str() + "-descr", "OBIS_IF_1107_RETRIES"),	//	retries
					cyng::param_factory(sml::OBIS_IF_1107_MIN_TIMEOUT.to_str() + "-descr", "OBIS_IF_1107_MIN_TIMEOUT"),	//	min. timeout (milliseconds)
					cyng::param_factory(sml::OBIS_IF_1107_MAX_TIMEOUT.to_str() + "-descr", "OBIS_IF_1107_MAX_TIMEOUT"),	//	max. timeout (milliseconds)
					cyng::param_factory(sml::OBIS_IF_1107_MAX_DATA_RATE.to_str() + "-descr", "OBIS_IF_1107_MAX_DATA_RATE"),	//	max. databytes
					cyng::param_factory(sml::OBIS_IF_1107_RS485.to_str() + "-descr", "OBIS_IF_1107_RS485"),	//	 true = RS485, false = RS232
					cyng::param_factory(sml::OBIS_IF_1107_PROTOCOL_MODE.to_str() + "-descr", "OBIS_IF_1107_PROTOCOL_MODE"),	//	protocol mode 0 == A, 1 == B, 2 == C (A...E)
					cyng::param_factory(sml::OBIS_IF_1107_AUTO_ACTIVATION.to_str() + "-descr", "OBIS_IF_1107_AUTO_ACTIVATION"),	//	auto activation
					cyng::param_factory(sml::OBIS_IF_1107_TIME_GRID.to_str() + "-descr", "OBIS_IF_1107_TIME_GRID"),
					cyng::param_factory(sml::OBIS_IF_1107_TIME_SYNC.to_str() + "-descr", "OBIS_IF_1107_TIME_SYNC"),
					cyng::param_factory(sml::OBIS_IF_1107_MAX_VARIATION.to_str() + "-descr", "OBIS_IF_1107_MAX_VARIATION"),	//	max. variation in seconds
#endif
					cyng::param_factory(sml::OBIS_IF_1107_ACTIVE.to_str(), true),	//	active
					cyng::param_factory(sml::OBIS_IF_1107_LOOP_TIME.to_str(), 60),	//	loop timeout in seconds
					cyng::param_factory(sml::OBIS_IF_1107_RETRIES.to_str(), 3),	//	retries
					cyng::param_factory(sml::OBIS_IF_1107_MIN_TIMEOUT.to_str(), 200),	//	min. timeout (milliseconds)
					cyng::param_factory(sml::OBIS_IF_1107_MAX_TIMEOUT.to_str(), 5000),	//	max. timeout (milliseconds)
					cyng::param_factory(sml::OBIS_IF_1107_MAX_DATA_RATE.to_str(), 10240),	//	max. databytes
					cyng::param_factory(sml::OBIS_IF_1107_RS485.to_str(), true),	//	 true = RS485, false = RS232
					cyng::param_factory(sml::OBIS_IF_1107_PROTOCOL_MODE.to_str(), 2),	//	protocol mode 0 == A, 1 == B, 2 == C (A...E)
					cyng::param_factory(sml::OBIS_IF_1107_AUTO_ACTIVATION.to_str(), true),	//	auto activation
					cyng::param_factory(sml::OBIS_IF_1107_TIME_GRID.to_str(), 900),
					cyng::param_factory(sml::OBIS_IF_1107_TIME_SYNC.to_str(), 14400),
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
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "26862"),
						cyng::param_factory("account", "gateway"),
						cyng::param_factory("pwd", "to-define"),
						cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::param_factory("scrambled", true),
						cyng::param_factory("monitor", rnd_monitor())),	//	seconds
					cyng::tuple_factory(
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "26863"),
						cyng::param_factory("account", "gateway"),
						cyng::param_factory("pwd", "to-define"),
						cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::param_factory("scrambled", false),
						cyng::param_factory("monitor", rnd_monitor()))
					}))

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

	bool controller::start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag)
	{
		//
		//  control push data logging
		//
		auto const log_pushdata = cyng::value_cast(cfg.get("log-pushdata"), false);
		boost::ignore_unused(log_pushdata);

		//
		//	SML login
		//
		auto const accept_all = cyng::value_cast(cfg.get("accept-all-ids"), false);
		if (accept_all) {
			CYNG_LOG_WARNING(logger, "Accepts all server IDs");
		}

		//
		//	map all available GPIO paths
		//
		auto const gpio_path = cyng::value_cast<std::string>(cfg.get("gpio-path"), "/sys/class/gpio");
		CYNG_LOG_INFO(logger, "gpio path: " << gpio_path);

		auto const gpio_list = cyng::vector_cast<int>(cfg.get("gpio-list"), 0);
		std::map<int, std::string> gpio_paths;
		for (auto const gpio : gpio_list) {
			std::stringstream ss;
			ss
				<< gpio_path
				<< "/gpio"
				<< gpio
				;
			auto const p = ss.str();
			CYNG_LOG_TRACE(logger, "gpio: " << p);
			gpio_paths.emplace(gpio, p);
		}

		if (gpio_paths.size() != 4) {
			CYNG_LOG_WARNING(logger, "invalid count of gpios: " << gpio_paths.size());
		}
		

		//
		//	get configuration type
		//
		//auto const config_types = cyng::vector_cast<std::string>(cfg.get("output"), "");

		//
		//	get IP-T configuration
		//
		cyng::vector_t vec;
		vec = cyng::value_cast(cfg.get("ipt"), vec);
		auto cfg_ipt = ipt::load_cluster_cfg(vec);

		//
		//	get wireless-LMN configuration
		//
		cyng::tuple_t cfg_wireless_lmn;
		cfg_wireless_lmn = cyng::value_cast(cfg.get("wireless-LMN"), cfg_wireless_lmn);

		//
		//	get wired-LMN configuration
		//
		cyng::tuple_t cfg_wired_lmn;
		cfg_wired_lmn = cyng::value_cast(cfg.get("wired-LMN"), cfg_wired_lmn);

		//
		//	get virtual meter
		//
		cyng::tuple_t cfg_virtual_meter;
		cfg_virtual_meter = cyng::value_cast(cfg.get("virtual-meter"), cfg_virtual_meter);

		/**
		 * global data cache
		 */
		cyng::store::db config_db;
		init_config(logger
			, config_db
			, tag
			, cfg);

		//
		//	connect to ipt master
		//
		cyng::vector_t tmp;
		join_network(mux
			, logger
			, config_db
			, tag
			, cfg_ipt
			, cfg_wireless_lmn
			, cfg_wired_lmn
			, cfg_virtual_meter
			, cyng::value_cast<std::string>(cfg["server"].get("account"), "")
			, cyng::value_cast<std::string>(cfg["server"].get("pwd"), "")
			, accept_all
			, gpio_paths);

		//
		//	create server
		//
		server srv(mux
			, logger
			, config_db
			, tag
			, cfg_ipt
			, cyng::value_cast<std::string>(cfg["server"].get("account"), "")
			, cyng::value_cast<std::string>(cfg["server"].get("pwd"), "")
			, accept_all);

		//
		//	server runtime configuration
		//
		const auto address = cyng::io::to_str(cfg["server"].get("address"));
		const auto service = cyng::io::to_str(cfg["server"].get("service"));

		CYNG_LOG_INFO(logger, "listener address: " << address);
		CYNG_LOG_INFO(logger, "listener service: " << service);
		srv.run(address, service);

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

	void join_network(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cyng::store::db& config_db
		, boost::uuids::uuid tag
		, ipt::redundancy const& cfg_ipt
		, cyng::tuple_t const& cfg_wireless_lmn
		, cyng::tuple_t const& cfg_wired_lmn
		, cyng::tuple_t const& cfg_virtual_meter
		, std::string account
		, std::string pwd
		, bool accept_all
		, std::map<int, std::string> gpio_list)
	{
		//CYNG_LOG_TRACE(logger, "network redundancy: " << cfg_ipt.size());

		cyng::async::start_task_delayed<ipt::network>(mux
			, std::chrono::seconds(1)
			, logger
			, config_db
			, tag
			, cfg_ipt
			, cfg_wireless_lmn
			, cfg_wired_lmn
			, account
			, pwd
			, accept_all
			, gpio_list);

		auto const dom = cyng::make_reader(cfg_virtual_meter);
		auto const b = cyng::value_cast(dom.get("enabled"), false);
		if (b) {

			//
			//	define virtual meter
			//
			auto const server = cyng::value_cast<std::string>(dom.get("server"), "01-d81c-10000001-3c-02");
			std::pair<cyng::buffer_t, bool> const r = sml::parse_srv_id(server);

			if (r.second) {

				auto const interval = cyng::numeric_cast<std::uint32_t>(dom.get("interval"), 26000ul);
				auto const active = cyng::value_cast(dom.get("active"), true);

				cyng::crypto::aes_128_key aes_key;
				cyng::crypto::aes::randomize(aes_key);
				
				if (config_db.insert("mbus-devices"
					, cyng::table::key_generator(r.first)
					, cyng::table::data_generator(std::chrono::system_clock::now()
						, "---"
						, active	//	active
						, "virtual meter"
						, 0ull	//	status
						, cyng::buffer_t{ 0, 0 }	//	mask
						, interval	//	interval
						, cyng::make_buffer({ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F })	//	pubKey
						, aes_key	//	random AES key
						, "user"
						, "pwd")
					, 1	//	generation
					, tag)) {

					//
					//	register some data collectors
					//
					std::uint8_t nr{ 0 };

					config_db.access([&](cyng::store::table* tbl) {


						tbl->insert(cyng::table::key_generator(r.first, ++nr)	//	key
							, cyng::table::data_generator(sml::OBIS_PROFILE_1_MINUTE.to_buffer(), false, static_cast<std::uint16_t>(1024u), std::chrono::seconds(0))
							, 1
							, tag);
						tbl->insert(cyng::table::key_generator(r.first, ++nr)	//	key
							, cyng::table::data_generator(sml::OBIS_PROFILE_15_MINUTE.to_buffer(), true, static_cast<std::uint16_t>(512u), std::chrono::seconds(0))
							, 1
							, tag);
						tbl->insert(cyng::table::key_generator(r.first, ++nr)	//	key
							, cyng::table::data_generator(sml::OBIS_PROFILE_60_MINUTE.to_buffer(), true, static_cast<std::uint16_t>(256u), std::chrono::seconds(0))
							, 1
							, tag);
						tbl->insert(cyng::table::key_generator(r.first, ++nr)	//	key
							, cyng::table::data_generator(sml::OBIS_PROFILE_24_HOUR.to_buffer(), true, static_cast<std::uint16_t>(128u), std::chrono::seconds(0))
							, 1
							, tag);
						tbl->insert(cyng::table::key_generator(r.first, ++nr)	//	key
							, cyng::table::data_generator(sml::OBIS_PROFILE_LAST_2_HOURS.to_buffer(), false, static_cast<std::uint16_t>(32u), std::chrono::seconds(0))
							, 1
							, tag);
						tbl->insert(cyng::table::key_generator(r.first, ++nr)	//	key
							, cyng::table::data_generator(sml::OBIS_PROFILE_LAST_WEEK.to_buffer(), false, static_cast<std::uint16_t>(32u), std::chrono::seconds(0))
							, 1
							, tag);
						tbl->insert(cyng::table::key_generator(r.first, ++nr)	//	key
							, cyng::table::data_generator(sml::OBIS_PROFILE_1_MONTH.to_buffer(), false, static_cast<std::uint16_t>(12u), std::chrono::seconds(0))
							, 1
							, tag);
						tbl->insert(cyng::table::key_generator(r.first, ++nr)	//	key
							, cyng::table::data_generator(sml::OBIS_PROFILE_1_YEAR.to_buffer(), false, static_cast<std::uint16_t>(2u), std::chrono::seconds(0))
							, 1
							, tag);

					}, cyng::store::write_access("data.collector"));

					//
					//	start virtual meter
					//
					cyng::async::start_task_delayed<virtual_meter>(mux
						, std::chrono::seconds(5)
						, logger
						, config_db
						, r.first	//	server ID
						, std::chrono::seconds(interval));
				}
				else {
					CYNG_LOG_WARNING(logger, "insert server ID: " << server << " into table \"mbus-devices\" failed");
				}
			}
			else {
				CYNG_LOG_WARNING(logger, "invalid server ID: " << server);

			}
		}
		else {
			CYNG_LOG_TRACE(logger, "virtual meter is not enabled");
		}
	}

	void init_config(cyng::logging::log_ptr logger
		, cyng::store::db& config
		, boost::uuids::uuid tag
		, cyng::reader<cyng::object> const& dom)
	{
		CYNG_LOG_TRACE(logger, "init configuration db");

		if (!create_table(config, "mbus-devices")) {
			CYNG_LOG_FATAL(logger, "cannot create table devices");
		}

		if (!create_table(config, "iec62056-21-devices")) {
			CYNG_LOG_FATAL(logger, "cannot create table iec62056-21-devices");
		}

		if (!create_table(config, "push.ops"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table push.ops");
		}

		if (!create_table(config, "op.log"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table op.log");
		}

		if (!create_table(config, "_Config"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table _Config");
		}
		else {
			config.access([&](cyng::store::table * tbl) {

				insert_config(tbl, "local.ep", boost::asio::ip::tcp::endpoint(), tag);
				insert_config(tbl, "remote.ep", boost::asio::ip::tcp::endpoint(), tag);

				insert_config(tbl, "gpio.46", std::size_t(cyng::async::NO_TASK), tag);
				insert_config(tbl, "gpio.47", std::size_t(cyng::async::NO_TASK), tag);
				insert_config(tbl, "gpio.50", std::size_t(cyng::async::NO_TASK), tag);
				insert_config(tbl, "gpio.53", std::size_t(cyng::async::NO_TASK), tag);

				//
				//	get hardware info
				//
				auto const manufacturer = cyng::value_cast<std::string>(dom["hardware"].get("manufacturer"), "solosTec");
				insert_config(tbl, sml::OBIS_DATA_MANUFACTURER.to_str(), manufacturer, tag);

				auto const model = cyng::value_cast<std::string>(dom["hardware"].get("model"), "Gateway");
				insert_config(tbl, OBIS_CODE(81, 81, c7, 82, 0a, 01).to_str(), model, tag);

				//
				//	generate even distributed integers
				//	reconnect to master on different times
				//
				cyng::crypto::rnd_num<int> rng(10, 120);

				//
				//	serial number = 32 bit unsigned
				//
				auto const serial = cyng::value_cast(dom["hardware"].get("serial"), rng());
				insert_config(tbl, OBIS_CODE(81, 81, c7, 82, 0a, 02).to_str(), serial, tag);

				std::string const dev_class = cyng::value_cast<std::string>(dom["hardware"].get("class"), "129-129:199.130.83*255");
				insert_config(tbl, sml::OBIS_DEVICE_CLASS.to_str(), dev_class, tag);

				//
				//	05 + MAC = server ID
				//
				std::string rnd_mac_str;
				{
					using cyng::io::operator<<;
					std::stringstream ss;
					ss << cyng::generate_random_mac48();
					ss >> rnd_mac_str;
				}
				auto const mac = cyng::value_cast<std::string>(dom["hardware"].get("mac"), rnd_mac_str);

				std::pair<cyng::mac48, bool > const r = cyng::parse_mac48(mac);


				CYNG_LOG_INFO(logger, "mac: " << mac);
				cyng::buffer_t srv_id = sml::to_gateway_srv_id(r.second ? r.first : cyng::generate_random_mac48());
				insert_config(tbl, sml::OBIS_SERVER_ID.to_str(), srv_id, tag);

				CYNG_LOG_INFO(logger, "manufacturer: " << manufacturer);
				CYNG_LOG_INFO(logger, "model: " << model);
				CYNG_LOG_INFO(logger, "device class: " << dev_class);
				CYNG_LOG_INFO(logger, "serial: " << serial);
				CYNG_LOG_INFO(logger, "server id: " << cyng::io::to_hex(srv_id));

				/**
				 * Global status word
				 * 459266:dec == ‭70202‬:hex == ‭01110000001000000010‬:bin
				 * OBIS_CLASS_OP_LOG_STATUS_WORD - 81 00 60 05 00 00
				 */
				sml::status_word_t word;
				sml::status status_word(word);
#ifdef _DEBUG
				status_word.set_mbus_if_available(true);
#endif
				insert_config(tbl, sml::OBIS_CLASS_OP_LOG_STATUS_WORD.to_str(), word, tag);


				//
				//	get if-1107 default configuration
				//
				auto const active = cyng::value_cast(dom["if-1107"].get(sml::OBIS_IF_1107_MAX_TIMEOUT.to_str()), true);
				CYNG_LOG_INFO(logger, sml::OBIS_IF_1107_ACTIVE.to_str() << " (OBIS_IF_1107_ACTIVE): " << (active ? "true" : "false"));
				insert_config(tbl, sml::OBIS_IF_1107_ACTIVE.to_str(), active, tag);

				auto const loop_time = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_IF_1107_LOOP_TIME.to_str()), 60u);
				CYNG_LOG_INFO(logger, sml::OBIS_IF_1107_LOOP_TIME.to_str() << " (OBIS_IF_1107_LOOP_TIME): " << loop_time);
				insert_config(tbl, sml::OBIS_IF_1107_LOOP_TIME.to_str(), loop_time, tag);

				auto const retries = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_IF_1107_RETRIES.to_str()), 3u);
				CYNG_LOG_INFO(logger, sml::OBIS_IF_1107_RETRIES.to_str() << " (OBIS_IF_1107_RETRIES): " << retries);
				insert_config(tbl, sml::OBIS_IF_1107_RETRIES.to_str(), retries, tag);

				auto const min_timeout = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_IF_1107_MIN_TIMEOUT.to_str()), 200u);
				CYNG_LOG_INFO(logger, sml::OBIS_IF_1107_MIN_TIMEOUT.to_str() << " (OBIS_IF_1107_MIN_TIMEOUT): " << min_timeout << " milliseconds");
				insert_config(tbl, sml::OBIS_IF_1107_MIN_TIMEOUT.to_str(), min_timeout, tag);

				auto const max_timeout = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_IF_1107_MAX_TIMEOUT.to_str()), 5000u);
				CYNG_LOG_INFO(logger, sml::OBIS_IF_1107_MAX_TIMEOUT.to_str() << " (OBIS_IF_1107_MAX_TIMEOUT): " << max_timeout << " milliseconds");
				insert_config(tbl, sml::OBIS_IF_1107_MAX_TIMEOUT.to_str(), max_timeout, tag);

				auto const max_data_rate = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_IF_1107_MAX_DATA_RATE.to_str()), 10240u);
				CYNG_LOG_INFO(logger, sml::OBIS_IF_1107_MAX_DATA_RATE.to_str() << " (OBIS_IF_1107_MAX_DATA_RATE): " << max_data_rate);
				insert_config(tbl, sml::OBIS_IF_1107_MAX_DATA_RATE.to_str(), max_data_rate, tag);

				auto const rs485 = cyng::value_cast(dom["if-1107"].get(sml::OBIS_IF_1107_RS485.to_str()), true);
				CYNG_LOG_INFO(logger, sml::OBIS_IF_1107_RS485.to_str() << " (OBIS_IF_1107_RS485): " << (rs485 ? "true" : "false"));
				insert_config(tbl, sml::OBIS_IF_1107_RS485.to_str(), rs485, tag);

				auto const protocol_mode = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_IF_1107_PROTOCOL_MODE.to_str()), 2u);
				if (protocol_mode > 4) {
					CYNG_LOG_WARNING(logger, sml::OBIS_IF_1107_PROTOCOL_MODE.to_str() << " (OBIS_IF_1107_PROTOCOL_MODE out of range): " << protocol_mode);
				}
				else {
					CYNG_LOG_INFO(logger, sml::OBIS_IF_1107_PROTOCOL_MODE.to_str() << " (OBIS_IF_1107_PROTOCOL_MODE): " << protocol_mode << " - " << char('A' + protocol_mode));
				}
				insert_config(tbl, sml::OBIS_IF_1107_PROTOCOL_MODE.to_str(), protocol_mode, tag);

				auto const auto_activation = cyng::value_cast(dom["if-1107"].get(sml::OBIS_IF_1107_AUTO_ACTIVATION.to_str()), true);
				CYNG_LOG_INFO(logger, sml::OBIS_IF_1107_AUTO_ACTIVATION.to_str() << " (OBIS_IF_1107_AUTO_ACTIVATION): " << (auto_activation ? "true" : "false"));
				insert_config(tbl, sml::OBIS_IF_1107_AUTO_ACTIVATION.to_str(), auto_activation, tag);

				auto const time_grid = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_IF_1107_TIME_GRID.to_str()), 900u);
				CYNG_LOG_INFO(logger, sml::OBIS_IF_1107_TIME_GRID.to_str() << " (OBIS_IF_1107_TIME_GRID): " << time_grid << " seconds, " << (time_grid / 60) << " minutes");
				insert_config(tbl, sml::OBIS_IF_1107_TIME_GRID.to_str(), time_grid, tag);

				auto const time_sync = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_IF_1107_TIME_SYNC.to_str()), 14400u);
				CYNG_LOG_INFO(logger, sml::OBIS_IF_1107_TIME_SYNC.to_str() << " (OBIS_IF_1107_TIME_SYNC): " << time_sync << " seconds, " << (time_sync / 3600) << " h");
				insert_config(tbl, sml::OBIS_IF_1107_TIME_SYNC.to_str(), time_sync, tag);

				auto const max_variation = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_IF_1107_MAX_VARIATION.to_str()), 9u);
				CYNG_LOG_INFO(logger, sml::OBIS_IF_1107_MAX_VARIATION.to_str() << " (OBIS_IF_1107_MAX_VARIATION): " << max_variation << " seconds");
				insert_config(tbl, sml::OBIS_IF_1107_MAX_VARIATION.to_str(), max_variation, tag);

				//
				//	get wireless M-Bus default configuration
				//
	// 			auto const radio_protocol = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_PROTOCOL.to_str()), static_cast<std::uint8_t>(1));
				auto const radio_protocol = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_PROTOCOL.to_str()), static_cast<std::uint8_t>(mbus::MODE_S));
				CYNG_LOG_INFO(logger, sml::OBIS_W_MBUS_PROTOCOL.to_str() << " (OBIS_W_MBUS_PROTOCOL): " << +radio_protocol);
				insert_config(tbl, sml::OBIS_W_MBUS_PROTOCOL.to_str(), radio_protocol, tag);

				auto const s_mode = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_MODE_S.to_str()), 30u);
				CYNG_LOG_INFO(logger, sml::OBIS_W_MBUS_MODE_S.to_str() << " (OBIS_W_MBUS_MODE_S): " << s_mode << " seconds");
				insert_config(tbl, sml::OBIS_W_MBUS_MODE_S.to_str(), s_mode, tag);

				auto const t_mode = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_MODE_T.to_str()), 20u);
				CYNG_LOG_INFO(logger, sml::OBIS_W_MBUS_MODE_T.to_str() << " (OBIS_W_MBUS_MODE_T): " << t_mode << " seconds");
				insert_config(tbl, sml::OBIS_W_MBUS_MODE_T.to_str(), t_mode, tag);

				auto const reboot = cyng::numeric_cast<std::uint32_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_REBOOT.to_str()), 20u);
				CYNG_LOG_INFO(logger, sml::OBIS_W_MBUS_REBOOT.to_str() << " (OBIS_W_MBUS_REBOOT): " << reboot << " seconds, " << (reboot / 3600) << " h");
				insert_config(tbl, sml::OBIS_W_MBUS_REBOOT.to_str(), reboot, tag);

				// 			auto const radio_power = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_POWER.to_str()), static_cast<std::uint8_t>(3));
				auto const radio_power = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_POWER.to_str()), static_cast<std::uint8_t>(mbus::STRONG));
				CYNG_LOG_INFO(logger, sml::OBIS_W_MBUS_POWER.to_str() << " (OBIS_W_MBUS_POWER): " << +radio_power);
				insert_config(tbl, sml::OBIS_W_MBUS_POWER.to_str(), radio_power, tag);

				auto const install_mode = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_INSTALL_MODE.to_str()), true);
				CYNG_LOG_INFO(logger, sml::OBIS_W_MBUS_INSTALL_MODE.to_str() << " (OBIS_W_MBUS_INSTALL_MODE): " << (install_mode ? "active" : "inactive"));
				insert_config(tbl, sml::OBIS_W_MBUS_INSTALL_MODE.to_str(), install_mode, tag);


				//
				//	get (wired) M-Bus default configuration
				//
				auto const ro_interval = cyng::numeric_cast<std::uint32_t>(dom["mbus"].get(sml::OBIS_CLASS_MBUS_RO_INTERVAL.to_str()), 3600);
				insert_config(tbl, sml::OBIS_CLASS_MBUS_RO_INTERVAL.to_str(), ro_interval, tag);

				auto const search_interval = cyng::numeric_cast<std::uint32_t>(dom["mbus"].get(sml::OBIS_CLASS_MBUS_SEARCH_INTERVAL.to_str()), 0);
				insert_config(tbl, sml::OBIS_CLASS_MBUS_SEARCH_INTERVAL.to_str(), search_interval, tag);

				auto const mbus_search_device = cyng::value_cast(dom["mbus"].get(sml::OBIS_CLASS_MBUS_SEARCH_DEVICE.to_str()), true);
				insert_config(tbl, sml::OBIS_CLASS_MBUS_SEARCH_DEVICE.to_str(), mbus_search_device, tag);

				auto const mbus_auto_activation = cyng::value_cast(dom["mbus"].get(sml::OBIS_CLASS_MBUS_AUTO_ACTICATE.to_str()), true);
				insert_config(tbl, sml::OBIS_CLASS_MBUS_AUTO_ACTICATE.to_str(), mbus_auto_activation, tag);

				auto const mbus_baud_rate = cyng::numeric_cast<std::uint32_t>(dom["mbus"].get(sml::OBIS_CLASS_MBUS_BITRATE.to_str()), 0);
				insert_config(tbl, sml::OBIS_CLASS_MBUS_BITRATE.to_str(), mbus_baud_rate, tag);


			}, cyng::store::write_access("_Config"));
		}

		if (!create_table(config, "readout"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table readout");
		}

		//
		//	create load profiles
		//
		if (!create_table(config, "profile.8181C78610FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table profile.8181C78610FF");
		}
		if (!create_table(config, "profile.8181C78611FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table profile.8181C78611FF");
		}
		if (!create_table(config, "profile.8181C78612FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table profile.8181C78612FF");
		}
		if (!create_table(config, "profile.8181C78613FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table profile.8181C78613FF");
		}
		if (!create_table(config, "profile.8181C78614FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table profile.8181C78614FF");
		}
		if (!create_table(config, "profile.8181C78615FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table profile.8181C78615FF");
		}
		if (!create_table(config, "profile.8181C78616FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table profile.8181C78616FF");
		}
		if (!create_table(config, "profile.8181C78617FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table profile.8181C78617FF");
		}
		if (!create_table(config, "profile.8181C78618FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table profile.8181C78618FF");
		}

		//
		//	create data storage
		//
		if (!create_table(config, "storage.8181C78610FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table storage.8181C78610FF");
		}
		if (!create_table(config, "storage.8181C78611FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table storage.8181C78611FF");
		}
		if (!create_table(config, "storage.8181C78612FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table storage.8181C78612FF");
		}
		if (!create_table(config, "storage.8181C78613FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table storage.8181C78613FF");
		}
		if (!create_table(config, "storage.8181C78614FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table storage.8181C78614FF");
		}
		if (!create_table(config, "storage.8181C78615FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table storage.8181C78615FF");
		}
		if (!create_table(config, "storage.8181C78616FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table storage.8181C78616FF");
		}
		if (!create_table(config, "storage.8181C78617FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table storage.8181C78617FF");
		}
		if (!create_table(config, "storage.8181C78618FF"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table storage.8181C78618FF");
		}

		if (!create_table(config, "data.collector"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table data.collector");
		}

		if (!create_table(config, "trx"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table trx");
		}
	}
}
