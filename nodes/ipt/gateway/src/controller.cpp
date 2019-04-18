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
#include "../../../../nodes/shared/db/db_meta.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/obis_io.h>
#include <smf/sml/obis_db.h>
#include <smf/sml/status.h>
#include <smf/sml/parser/srv_id_parser.h>
#include <smf/mbus/defs.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/async/signal_handler.h>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
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

#if BOOST_OS_WINDOWS
#include <cyng/scm/service.hpp>
#endif
#if BOOST_OS_LINUX
#include "../../../write_pid.h"
#endif
#include <fstream>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/core/ignore_unused.hpp>

namespace node
{
	//
	//	forward declaration(s):
	//
	bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::object);
	bool wait(cyng::logging::log_ptr logger);
	void join_network(cyng::async::mux&
		, cyng::logging::log_ptr
		, sml::status&
		, cyng::store::db&
		, boost::uuids::uuid tag
		, ipt::master_config_t const& cfg_ipt
		, cyng::tuple_t const& cfg_wireless_lmn
		, cyng::tuple_t const& cfg_wired_lmn
		, cyng::tuple_t const& cfg_virtual_meter
		, std::string account
		, std::string pwd
		, std::string manufacturer
		, std::string model
		, std::uint32_t serial
		, cyng::mac48
		, bool accept_all
		, std::map<int, std::string> gpio_list);
	void init_config(cyng::logging::log_ptr logger
		, cyng::store::db&
		, boost::uuids::uuid
		, cyng::mac48
		, cyng::reader<cyng::object> const&);

	controller::controller(unsigned int pool_size, std::string const& json_path)
		: pool_size_(pool_size)
		, json_path_(json_path)
	{}

	int controller::run(bool console)
	{
		//
		//	to calculate uptime
		//
		const std::chrono::system_clock::time_point tp_start = std::chrono::system_clock::now();
		//
		//	controller loop
		//
		try
		{
			//
			//	controller loop
			//
			bool shutdown{ false };
			while (!shutdown)
			{
				//
				//	establish I/O context
				//
				cyng::async::mux mux{ this->pool_size_ };

				//
				//	read configuration file
				//
				cyng::object config = cyng::json::read_file(json_path_);
				if (config)
				{
					//
					//	start application
					//
					cyng::vector_t vec;
					vec = cyng::value_cast(config, vec);

					if (vec.empty())
					{
						std::cerr
							<< "use option -D to generate a configuration file"
							<< std::endl;
						shutdown = true;
					}
					else
					{
						//
						//	initialize logger
						//
#if BOOST_OS_LINUX
						auto logger = cyng::logging::make_sys_logger("ipt:gateway", true);
#else
						const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
						auto dom = cyng::make_reader(vec[0]);
						const boost::filesystem::path log_dir = cyng::value_cast(dom.get("log-dir"), tmp.string());

						auto logger = (console)
							? cyng::logging::make_console_logger(mux.get_io_service(), "ipt:gateway")
							: cyng::logging::make_file_logger(mux.get_io_service(), (log_dir / "ipt-gateway.log"))
							;
#ifdef _DEBUG
						if (!console) {
							std::cout << "log file see: " << (log_dir / "ipt-gateway.log") << std::endl;
						}
#endif
#endif

						CYNG_LOG_TRACE(logger, cyng::io::to_str(config));
						CYNG_LOG_INFO(logger, "pool size: " << this->pool_size_);

						//
						//	start 
						//
						shutdown = start(mux, logger, vec[0]);

						//
						//	print uptime
						//
						const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tp_start);
						CYNG_LOG_INFO(logger, "ipt:gateway uptime " << cyng::io::to_str(cyng::make_object(duration)));
					}
				}
				else
				{
					std::cout
						<< "use option -D to generate a configuration file"
						<< std::endl;

					//
					//	no configuration found
					//
					shutdown = true;
				}

				//
				//	shutdown scheduler
				//
				mux.shutdown();
			}

			return EXIT_SUCCESS;
		}
		catch (std::exception& ex)
		{
			std::cerr
				<< ex.what()
				<< std::endl;
		}

		return EXIT_FAILURE;
	}

	int controller::create_config() const
	{
		std::fstream fout(json_path_, std::ios::trunc | std::ios::out);
		if (fout.is_open())
		{
			std::cout
				<< "write to file "
				<< json_path_
				<< std::endl;
			//
			//	get default values
			//
			const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
			const boost::filesystem::path pwd = boost::filesystem::current_path();

			//
			//	random UUID
			//
			boost::uuids::random_generator uidgen;

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

			const auto conf = cyng::vector_factory({
				cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level", "INFO")
					, cyng::param_factory("tag", uidgen())
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
						cyng::param_factory("manufacturer", "solosTec"),	//	manufacturer (81 81 C7 82 03 FF)
						cyng::param_factory("model", "virtual.gateway"),	//	Typenschlüssel (81 81 C7 82 09 FF --> 81 81 C7 82 0A 01)
						cyng::param_factory("serial", sn),	//	Seriennummer (81 81 C7 82 09 FF --> 81 81 C7 82 0A 02)
						cyng::param_factory("class", "129-129:199.130.83*255"),	//	device class (81 81 C7 82 02 FF) MUC-LAN/DSL
						cyng::param_factory("mac", macs.at(0))	//	take first available MAC to build a server id (05 xx xx ...)
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
						cyng::param_factory(sml::OBIS_CODE_IF_1107_ACTIVE.to_str() + "-descr", "OBIS_CODE_IF_1107_ACTIVE"),	//	active
						cyng::param_factory(sml::OBIS_CODE_IF_1107_LOOP_TIME.to_str() + "-descr", "OBIS_CODE_IF_1107_LOOP_TIME"),	//	loop timeout in seconds
						cyng::param_factory(sml::OBIS_CODE_IF_1107_RETRIES.to_str() + "-descr", "OBIS_CODE_IF_1107_RETRIES"),	//	retries
						cyng::param_factory(sml::OBIS_CODE_IF_1107_MIN_TIMEOUT.to_str() + "-descr", "OBIS_CODE_IF_1107_MIN_TIMEOUT"),	//	min. timeout (milliseconds)
						cyng::param_factory(sml::OBIS_CODE_IF_1107_MAX_TIMEOUT.to_str() + "-descr", "OBIS_CODE_IF_1107_MAX_TIMEOUT"),	//	max. timeout (milliseconds)
						cyng::param_factory(sml::OBIS_CODE_IF_1107_MAX_DATA_RATE.to_str() + "-descr", "OBIS_CODE_IF_1107_MAX_DATA_RATE"),	//	max. databytes
						cyng::param_factory(sml::OBIS_CODE_IF_1107_RS485.to_str() + "-descr", "OBIS_CODE_IF_1107_RS485"),	//	 true = RS485, false = RS232
						cyng::param_factory(sml::OBIS_CODE_IF_1107_PROTOCOL_MODE.to_str() + "-descr", "OBIS_CODE_IF_1107_PROTOCOL_MODE"),	//	protocol mode 0 == A, 1 == B, 2 == C (A...E)
						cyng::param_factory(sml::OBIS_CODE_IF_1107_AUTO_ACTIVATION.to_str() + "-descr", "OBIS_CODE_IF_1107_AUTO_ACTIVATION"),	//	auto activation
						cyng::param_factory(sml::OBIS_CODE_IF_1107_TIME_GRID.to_str() + "-descr", "OBIS_CODE_IF_1107_TIME_GRID"),
						cyng::param_factory(sml::OBIS_CODE_IF_1107_TIME_SYNC.to_str() + "-descr", "OBIS_CODE_IF_1107_TIME_SYNC"),
						cyng::param_factory(sml::OBIS_CODE_IF_1107_MAX_VARIATION.to_str() + "-descr", "OBIS_CODE_IF_1107_MAX_VARIATION"),	//	max. variation in seconds
#endif
						cyng::param_factory(sml::OBIS_CODE_IF_1107_ACTIVE.to_str(), true),	//	active
						cyng::param_factory(sml::OBIS_CODE_IF_1107_LOOP_TIME.to_str(), 60),	//	loop timeout in seconds
						cyng::param_factory(sml::OBIS_CODE_IF_1107_RETRIES.to_str(), 3),	//	retries
						cyng::param_factory(sml::OBIS_CODE_IF_1107_MIN_TIMEOUT.to_str(), 200),	//	min. timeout (milliseconds)
						cyng::param_factory(sml::OBIS_CODE_IF_1107_MAX_TIMEOUT.to_str(), 5000),	//	max. timeout (milliseconds)
						cyng::param_factory(sml::OBIS_CODE_IF_1107_MAX_DATA_RATE.to_str(), 10240),	//	max. databytes
						cyng::param_factory(sml::OBIS_CODE_IF_1107_RS485.to_str(), true),	//	 true = RS485, false = RS232
						cyng::param_factory(sml::OBIS_CODE_IF_1107_PROTOCOL_MODE.to_str(), 2),	//	protocol mode 0 == A, 1 == B, 2 == C (A...E)
						cyng::param_factory(sml::OBIS_CODE_IF_1107_AUTO_ACTIVATION.to_str(), true),	//	auto activation
						cyng::param_factory(sml::OBIS_CODE_IF_1107_TIME_GRID.to_str(), 900),
						cyng::param_factory(sml::OBIS_CODE_IF_1107_TIME_SYNC.to_str(), 14400),
						cyng::param_factory(sml::OBIS_CODE_IF_1107_MAX_VARIATION.to_str(), 9)	//	max. variation in seconds

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

			cyng::json::write(std::cout, cyng::make_object(conf));
			std::cout << std::endl;
			cyng::json::write(fout, cyng::make_object(conf));
			return EXIT_SUCCESS;
		}
		else
		{
			std::cerr
				<< "unable to open file "
				<< json_path_
				<< std::endl;

		}
		return EXIT_FAILURE;
	}

#if BOOST_OS_WINDOWS
	int controller::run_as_service(controller&& ctrl, std::string const& srv_name)
	{
		//
		//	define service type
		//
		typedef service< controller >	service_type;

		//
		//	create service
		//
		::OutputDebugString(srv_name.c_str());
		service_type srv(std::move(ctrl), srv_name);

		//
		//	starts dispatcher and calls service main() function 
		//
		const DWORD r = srv.run();
		switch (r)
		{
		case ERROR_SERVICE_ALREADY_RUNNING:
			//	An instance of the service is already running.
			::OutputDebugString("An instance of the [ipt:gateway] service is already running.");
			break;
		case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
			//
			//	The service process could not connect to the service controller.
			//	Typical error message, when running in console mode.
			//
			::OutputDebugString("***Error 1063: The [ipt:gateway] service process could not connect to the service controller.");
			std::cerr
				<< "***Error 1063: The [e350] service could not connect to the service controller."
				<< std::endl
				;
			break;
		case ERROR_SERVICE_NOT_IN_EXE:
			//	The executable program that this service is configured to run in does not implement the service.
			::OutputDebugString("The [ipt:gateway] service is configured to run in does not implement the service.");
			break;
		default:
		{
			std::stringstream ss;
			ss
				<< '['
				<< srv_name
				<< "] service dispatcher stopped "
				<< r;
			const std::string msg = ss.str();
			::OutputDebugString(msg.c_str());
		}
		break;
		}


		return EXIT_SUCCESS;
	}

	void controller::control_handler(DWORD sig)
	{
		//	forward signal to shutdown manager
		cyng::forward_signal(sig);
	}

#endif


	bool start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::object cfg)
	{
		CYNG_LOG_TRACE(logger, cyng::dom_counter(cfg) << " configuration nodes found");

		//
		//	provide a configuration reader
		//
		auto const dom = cyng::make_reader(cfg);

		//
		//	random UUID
		//
		boost::uuids::random_generator uidgen;
		auto const tag = cyng::value_cast<boost::uuids::uuid>(dom.get("tag"), uidgen());

		//
		//	generate even distributed integers
		//	reconnect to master on different times
		//
		cyng::crypto::rnd_num<int> rng(10, 120);

		//
		//	apply severity threshold
		//
		logger->set_severity(cyng::logging::to_severity(cyng::value_cast<std::string>(dom.get("log-level"), "INFO")));

#if BOOST_OS_LINUX
		const boost::filesystem::path log_dir = cyng::value_cast<std::string>(dom.get("log-dir"), ".");
		write_pid(log_dir, tag);
#endif

		//
		//  control push data logging
		//
		auto const log_pushdata = cyng::value_cast(dom.get("log-pushdata"), false);
		boost::ignore_unused(log_pushdata);

		//
		//	SML login
		//
		auto const accept_all = cyng::value_cast(dom.get("accept-all-ids"), false);
		if (accept_all) {
			CYNG_LOG_WARNING(logger, "Accepts all server IDs");
		}

		//
		//	map all available GPIO paths
		//
		auto const gpio_path = cyng::value_cast<std::string>(dom.get("gpio-path"), "/sys/class/gpio");
		CYNG_LOG_INFO(logger, "gpio path: " << gpio_path);

		auto const gpio_list = cyng::vector_cast<int>(dom.get("gpio-list"), 0);
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
		auto const config_types = cyng::vector_cast<std::string>(dom.get("output"), "");

		//
		//	get hardware info
		//
		std::string const manufacturer = cyng::value_cast<std::string>(dom["hardware"].get("manufacturer"), "solosTec");
		std::string const model = cyng::value_cast<std::string>(dom["hardware"].get("model"), "Gateway");

		//
		//	serial number = 32 bit unsigned
		//
		auto const serial = cyng::value_cast(dom["hardware"].get("serial"), rng());

		std::string const dev_class = cyng::value_cast<std::string>(dom["hardware"].get("class"), "129-129:199.130.83*255");

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


		CYNG_LOG_INFO(logger, "manufacturer: " << manufacturer);
		CYNG_LOG_INFO(logger, "model: " << model);
		CYNG_LOG_INFO(logger, "dev_class: " << dev_class);
		CYNG_LOG_INFO(logger, "serial: " << serial);
		CYNG_LOG_INFO(logger, "mac: " << mac);
		

		/**
		 * Global status word
		 * 459266:dec == ‭70202‬:hex == ‭01110000001000000010‬:bin
		 */
		sml::status status_word;
#ifdef _DEBUG
		status_word.set_mbus_if_available(true);
#endif

		//
		//	get IP-T configuration
		//
		cyng::vector_t vec;
		vec = cyng::value_cast(dom.get("ipt"), vec);
		auto cfg_ipt = ipt::load_cluster_cfg(vec);

		//
		//	get wireless-LMN configuration
		//
		cyng::tuple_t cfg_wireless_lmn;
		cfg_wireless_lmn = cyng::value_cast(dom.get("wireless-LMN"), cfg_wireless_lmn);

		//
		//	get wired-LMN configuration
		//
		cyng::tuple_t cfg_wired_lmn;
		cfg_wired_lmn = cyng::value_cast(dom.get("wired-LMN"), cfg_wired_lmn);

		//
		//	get virtual meter
		//
		cyng::tuple_t cfg_virtual_meter;
		cfg_virtual_meter = cyng::value_cast(dom.get("virtual-meter"), cfg_virtual_meter);

		/**
		 * global data cache
		 */
		cyng::store::db config_db;
		init_config(logger, config_db, tag, r.first, dom);

		//
		//	connect to ipt master
		//
		cyng::vector_t tmp;
		join_network(mux
			, logger
			, status_word
			, config_db
			, tag
			, cfg_ipt
			, cfg_wireless_lmn
			, cfg_wired_lmn
			, cfg_virtual_meter
			, cyng::value_cast<std::string>(dom["server"].get("account"), "")
			, cyng::value_cast<std::string>(dom["server"].get("pwd"), "")
			, manufacturer
			, model
			, serial
			, r.first
			, accept_all
			, gpio_paths);

		//
		//	create server
		//
		server srv(mux
			, logger
			, status_word
			, config_db
			, tag
			, cfg_ipt
			, cyng::value_cast<std::string>(dom["server"].get("account"), "")
			, cyng::value_cast<std::string>(dom["server"].get("pwd"), "")
			, manufacturer
			, model
			, serial
			, r.first
			, accept_all);

		//
		//	server runtime configuration
		//
		const auto address = cyng::io::to_str(dom["server"].get("address"));
		const auto service = cyng::io::to_str(dom["server"].get("service"));

		CYNG_LOG_INFO(logger, "listener address: " << address);
		CYNG_LOG_INFO(logger, "listener service: " << service);
		srv.run(address, service);

		//
		//	wait for system signals
		//
		const bool shutdown = wait(logger);

		//
		//	close acceptor
		//
		CYNG_LOG_INFO(logger, "close acceptor");
		srv.close();

		//
		//	stop all connections
		//
		CYNG_LOG_INFO(logger, "stop all connections");

		//
		//	stop all tasks
		//
		CYNG_LOG_INFO(logger, "stop all tasks");
		mux.stop();

		return shutdown;
	}

	bool wait(cyng::logging::log_ptr logger)
	{
		//
		//	wait for system signals
		//
		bool shutdown = false;
		cyng::signal_mgr signal;
		switch (signal.wait())
		{
#if BOOST_OS_WINDOWS
		case CTRL_BREAK_EVENT:
#else
		case SIGHUP:
#endif
			CYNG_LOG_INFO(logger, "SIGHUP received");
			break;
		default:
			CYNG_LOG_WARNING(logger, "SIGINT received");
			shutdown = true;
			break;
		}
		return shutdown;
	}


	void join_network(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, sml::status& status
		, cyng::store::db& config_db
		, boost::uuids::uuid tag
		, ipt::master_config_t const& cfg_ipt
		, cyng::tuple_t const& cfg_wireless_lmn
		, cyng::tuple_t const& cfg_wired_lmn
		, cyng::tuple_t const& cfg_virtual_meter
		, std::string account
		, std::string pwd
		, std::string manufacturer
		, std::string model
		, std::uint32_t serial
		, cyng::mac48 mac
		, bool accept_all
		, std::map<int, std::string> gpio_list)
	{
		CYNG_LOG_TRACE(logger, "network redundancy: " << cfg_ipt.size());

		cyng::async::start_task_delayed<ipt::network>(mux
			, std::chrono::seconds(1)
			, logger
			, status
			, config_db
			, tag
			, cfg_ipt
			, cfg_wireless_lmn
			, cfg_wired_lmn
			, account
			, pwd
			, manufacturer
			, model
			, serial
			, mac
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

					config_db.insert("data.collector"
						, cyng::table::key_generator(r.first, ++nr)	//	key
						, cyng::table::data_generator(sml::OBIS_PROFILE_1_MINUTE.to_buffer(), false, static_cast<std::uint16_t>(1024u), std::chrono::seconds(0))
						, 1
						, tag);
					config_db.insert("data.collector"
						, cyng::table::key_generator(r.first, ++nr)	//	key
						, cyng::table::data_generator(sml::OBIS_PROFILE_15_MINUTE.to_buffer(), true, static_cast<std::uint16_t>(512u), std::chrono::seconds(0))
						, 1
						, tag);
					config_db.insert("data.collector"
						, cyng::table::key_generator(r.first, ++nr)	//	key
						, cyng::table::data_generator(sml::OBIS_PROFILE_60_MINUTE.to_buffer(), true, static_cast<std::uint16_t>(256u), std::chrono::seconds(0))
						, 1
						, tag);
					config_db.insert("data.collector"
						, cyng::table::key_generator(r.first, ++nr)	//	key
						, cyng::table::data_generator(sml::OBIS_PROFILE_24_HOUR.to_buffer(), true, static_cast<std::uint16_t>(128u), std::chrono::seconds(0))
						, 1
						, tag);
					config_db.insert("data.collector"
						, cyng::table::key_generator(r.first, ++nr)	//	key
						, cyng::table::data_generator(sml::OBIS_PROFILE_LAST_2_HOURS.to_buffer(), false, static_cast<std::uint16_t>(32u), std::chrono::seconds(0))
						, 1
						, tag);
					config_db.insert("data.collector"
						, cyng::table::key_generator(r.first, ++nr)	//	key
						, cyng::table::data_generator(sml::OBIS_PROFILE_LAST_WEEK.to_buffer(), false, static_cast<std::uint16_t>(32u), std::chrono::seconds(0))
						, 1
						, tag);
					config_db.insert("data.collector"
						, cyng::table::key_generator(r.first, ++nr)	//	key
						, cyng::table::data_generator(sml::OBIS_PROFILE_1_MONTH.to_buffer(), false, static_cast<std::uint16_t>(12u), std::chrono::seconds(0))
						, 1
						, tag);
					config_db.insert("data.collector"
						, cyng::table::key_generator(r.first, ++nr)	//	key
						, cyng::table::data_generator(sml::OBIS_PROFILE_1_YEAR.to_buffer(), false, static_cast<std::uint16_t>(2u), std::chrono::seconds(0))
						, 1
						, tag);


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
		, cyng::mac48 mac
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
			config.insert("_Config", cyng::table::key_generator("local.ep"), cyng::table::data_generator(boost::asio::ip::tcp::endpoint()), 1, tag);
			config.insert("_Config", cyng::table::key_generator("remote.ep"), cyng::table::data_generator(boost::asio::ip::tcp::endpoint()), 1, tag);

			config.insert("_Config", cyng::table::key_generator("gpio.46"), cyng::table::data_generator(std::size_t(cyng::async::NO_TASK)), 1, tag);
			config.insert("_Config", cyng::table::key_generator("gpio.47"), cyng::table::data_generator(std::size_t(cyng::async::NO_TASK)), 1, tag);
			config.insert("_Config", cyng::table::key_generator("gpio.50"), cyng::table::data_generator(std::size_t(cyng::async::NO_TASK)), 1, tag);
			config.insert("_Config", cyng::table::key_generator("gpio.53"), cyng::table::data_generator(std::size_t(cyng::async::NO_TASK)), 1, tag);

			//
			//	get if-1107 default configuration
			//
			auto const active = cyng::value_cast(dom["if-1107"].get(sml::OBIS_CODE_IF_1107_MAX_TIMEOUT.to_str()), true);
			CYNG_LOG_INFO(logger, sml::OBIS_CODE_IF_1107_ACTIVE.to_str() << " (OBIS_CODE_IF_1107_ACTIVE): " << (active ? "true" : "false"));
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_CODE_IF_1107_ACTIVE.to_str()), cyng::table::data_generator(active), 1, tag);

			auto const loop_time = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_CODE_IF_1107_LOOP_TIME.to_str()), 60u);
			CYNG_LOG_INFO(logger, sml::OBIS_CODE_IF_1107_LOOP_TIME.to_str() << " (OBIS_CODE_IF_1107_LOOP_TIME): " << loop_time);
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_CODE_IF_1107_LOOP_TIME.to_str()), cyng::table::data_generator(loop_time), 1, tag);

			auto const retries = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_CODE_IF_1107_RETRIES.to_str()), 3u);
			CYNG_LOG_INFO(logger, sml::OBIS_CODE_IF_1107_RETRIES.to_str() << " (OBIS_CODE_IF_1107_RETRIES): " << retries);
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_CODE_IF_1107_RETRIES.to_str()), cyng::table::data_generator(retries), 1, tag);

			auto const min_timeout = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_CODE_IF_1107_MIN_TIMEOUT.to_str()), 200u);
			CYNG_LOG_INFO(logger, sml::OBIS_CODE_IF_1107_MIN_TIMEOUT.to_str() << " (OBIS_CODE_IF_1107_MIN_TIMEOUT): " << min_timeout << " milliseconds");
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_CODE_IF_1107_MIN_TIMEOUT.to_str()), cyng::table::data_generator(min_timeout), 1, tag);

			auto const max_timeout = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_CODE_IF_1107_MAX_TIMEOUT.to_str()), 5000u);
			CYNG_LOG_INFO(logger, sml::OBIS_CODE_IF_1107_MAX_TIMEOUT.to_str() << " (OBIS_CODE_IF_1107_MAX_TIMEOUT): " << max_timeout << " milliseconds");
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_CODE_IF_1107_MAX_TIMEOUT.to_str()), cyng::table::data_generator(max_timeout), 1, tag);

			auto const max_data_rate = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_CODE_IF_1107_MAX_DATA_RATE.to_str()), 10240u);
			CYNG_LOG_INFO(logger, sml::OBIS_CODE_IF_1107_MAX_DATA_RATE.to_str() << " (OBIS_CODE_IF_1107_MAX_DATA_RATE): " << max_data_rate);
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_CODE_IF_1107_MAX_DATA_RATE.to_str()), cyng::table::data_generator(max_data_rate), 1, tag);

			auto const rs485 = cyng::value_cast(dom["if-1107"].get(sml::OBIS_CODE_IF_1107_RS485.to_str()), true);
			CYNG_LOG_INFO(logger, sml::OBIS_CODE_IF_1107_RS485.to_str() << " (OBIS_CODE_IF_1107_RS485): " << (rs485 ? "true" : "false"));
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_CODE_IF_1107_RS485.to_str()), cyng::table::data_generator(rs485), 1, tag);

			auto const protocol_mode = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_CODE_IF_1107_PROTOCOL_MODE.to_str()), 2u);
			if (protocol_mode > 4) {
				CYNG_LOG_WARNING(logger, sml::OBIS_CODE_IF_1107_PROTOCOL_MODE.to_str() << " (OBIS_CODE_IF_1107_PROTOCOL_MODE out of range): " << protocol_mode);
			}
			else {
				CYNG_LOG_INFO(logger, sml::OBIS_CODE_IF_1107_PROTOCOL_MODE.to_str() << " (OBIS_CODE_IF_1107_PROTOCOL_MODE): " << protocol_mode << " - " << char('A' + protocol_mode));
			}
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_CODE_IF_1107_PROTOCOL_MODE.to_str()), cyng::table::data_generator(protocol_mode), 1, tag);

			auto const auto_activation = cyng::value_cast(dom["if-1107"].get(sml::OBIS_CODE_IF_1107_AUTO_ACTIVATION.to_str()), true);
			CYNG_LOG_INFO(logger, sml::OBIS_CODE_IF_1107_AUTO_ACTIVATION.to_str() << " (OBIS_CODE_IF_1107_AUTO_ACTIVATION): " << (auto_activation ? "true" : "false"));
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_CODE_IF_1107_AUTO_ACTIVATION.to_str()), cyng::table::data_generator(auto_activation), 1, tag);

			auto const time_grid = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_CODE_IF_1107_TIME_GRID.to_str()), 900u);
			CYNG_LOG_INFO(logger, sml::OBIS_CODE_IF_1107_TIME_GRID.to_str() << " (OBIS_CODE_IF_1107_TIME_GRID): " << time_grid << " seconds, " << (time_grid / 60) << " minutes");
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_CODE_IF_1107_TIME_GRID.to_str()), cyng::table::data_generator(time_grid), 1, tag);

			auto const time_sync = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_CODE_IF_1107_TIME_SYNC.to_str()), 14400u);
			CYNG_LOG_INFO(logger, sml::OBIS_CODE_IF_1107_TIME_SYNC.to_str() << " (OBIS_CODE_IF_1107_TIME_SYNC): " << time_sync << " seconds, " << (time_sync / 3600) << " h");
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_CODE_IF_1107_TIME_SYNC.to_str()), cyng::table::data_generator(time_sync), 1, tag);

			auto const max_variation = cyng::numeric_cast(dom["if-1107"].get(sml::OBIS_CODE_IF_1107_MAX_VARIATION.to_str()), 9u);
			CYNG_LOG_INFO(logger, sml::OBIS_CODE_IF_1107_MAX_VARIATION.to_str() << " (OBIS_CODE_IF_1107_MAX_VARIATION): " << max_variation << " seconds");
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_CODE_IF_1107_MAX_VARIATION.to_str()), cyng::table::data_generator(max_variation), 1, tag);

			//
			//	get wireless M-Bus default configuration
			//
// 			auto const radio_protocol = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_PROTOCOL.to_str()), static_cast<std::uint8_t>(1));
			auto const radio_protocol = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_PROTOCOL.to_str()), static_cast<std::uint8_t>(mbus::MODE_S));
			CYNG_LOG_INFO(logger, sml::OBIS_W_MBUS_PROTOCOL.to_str() << " (OBIS_W_MBUS_PROTOCOL): " << +radio_protocol);
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_W_MBUS_PROTOCOL.to_str()), cyng::table::data_generator(radio_protocol), 1, tag);

			auto const s_mode = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_MODE_S.to_str()), 30u);
			CYNG_LOG_INFO(logger, sml::OBIS_W_MBUS_MODE_S.to_str() << " (OBIS_W_MBUS_MODE_S): " << s_mode << " seconds");
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_W_MBUS_MODE_S.to_str()), cyng::table::data_generator(s_mode), 1, tag);

			auto const t_mode = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_MODE_T.to_str()), 20u);
			CYNG_LOG_INFO(logger, sml::OBIS_W_MBUS_MODE_T.to_str() << " (OBIS_W_MBUS_MODE_T): " << t_mode << " seconds");
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_W_MBUS_MODE_T.to_str()), cyng::table::data_generator(t_mode), 1, tag);

			auto const reboot = cyng::numeric_cast<std::uint32_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_REBOOT.to_str()), 20u);
			CYNG_LOG_INFO(logger, sml::OBIS_W_MBUS_REBOOT.to_str() << " (OBIS_W_MBUS_REBOOT): " << reboot << " seconds, " << (reboot / 3600) << " h");
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_W_MBUS_REBOOT.to_str()), cyng::table::data_generator(reboot), 1, tag);

// 			auto const radio_power = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_POWER.to_str()), static_cast<std::uint8_t>(3));
			auto const radio_power = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_POWER.to_str()), static_cast<std::uint8_t>(mbus::STRONG));
			CYNG_LOG_INFO(logger, sml::OBIS_W_MBUS_POWER.to_str() << " (OBIS_W_MBUS_POWER): " << +radio_power);
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_W_MBUS_POWER.to_str()), cyng::table::data_generator(radio_power), 1, tag);

			auto const install_mode = cyng::numeric_cast<std::uint8_t>(dom["wired-LMN"].get(sml::OBIS_W_MBUS_INSTALL_MODE.to_str()), true);
			CYNG_LOG_INFO(logger, sml::OBIS_W_MBUS_INSTALL_MODE.to_str() << " (OBIS_W_MBUS_INSTALL_MODE): " << (install_mode ? "active" : "inactive"));
			config.insert("_Config", cyng::table::key_generator(sml::OBIS_W_MBUS_INSTALL_MODE.to_str()), cyng::table::data_generator(install_mode), 1, tag);


		}

		if (!create_table(config, "readout"))
		{
			CYNG_LOG_FATAL(logger, "cannot create table readout");
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
