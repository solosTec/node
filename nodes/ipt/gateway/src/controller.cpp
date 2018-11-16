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
#include "../../../../nodes/shared/db/db_meta.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/status.h>

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
#include <cyng/set_cast.h>
#include <cyng/vector_cast.hpp>
#include <cyng/sys/mac.h>
#include <cyng/store/db.h>
#include <cyng/table/meta.hpp>
#include <cyng/rnd.h>
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
		, cyng::tuple_t const& cfg_wmbus
		, std::string account
		, std::string pwd
		, std::string manufacturer
		, std::string model
		, std::uint32_t serial
		, cyng::mac48);
	void init_config(cyng::logging::log_ptr logger, cyng::store::db&, boost::uuids::uuid, cyng::mac48);

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
						CYNG_LOG_INFO(logger, "uptime " << cyng::io::to_str(cyng::make_object(duration)));
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

					//	on this address the gateway acts as a server
					//	configuration interface
					, cyng::param_factory("server", cyng::tuple_factory(
						cyng::param_factory("address", "0.0.0.0"),
						cyng::param_factory("service", "7259"),
						cyng::param_factory("discover", "5798"),	//	UDP
						cyng::param_factory("account", "operator"),
						cyng::param_factory("pwd", "operator")
					))

					//	data interface
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
					, cyng::param_factory("wMBus", cyng::tuple_factory(
#if BOOST_OS_WINDOWS
						cyng::param_factory("enabled", false),
#else
						cyng::param_factory("enabled", true),
#endif
						cyng::param_factory("input", "/dev/ttyAPP0"),
						cyng::param_factory("speed", 115200)
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
		auto dom = cyng::make_reader(cfg);

		//
		//	random UUID
		//
		boost::uuids::random_generator uidgen;
		const auto tag = cyng::value_cast<boost::uuids::uuid>(dom.get("tag"), uidgen());

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
		const auto log_pushdata = cyng::value_cast(dom.get("log-pushdata"), false);
		boost::ignore_unused(log_pushdata);

		//
		//	get configuration type
		//
		const auto config_types = cyng::vector_cast<std::string>(dom.get("output"), "");

		//
		//	get hardware info
		//
		const std::string manufacturer = cyng::value_cast<std::string>(dom["hardware"].get("manufacturer"), "solosTec");
		const std::string model = cyng::value_cast<std::string>(dom["hardware"].get("model"), "Gateway");

		//
		//	serial number = 32 bit unsigned
		//
		const auto serial = cyng::value_cast(dom["hardware"].get("serial"), rng());

		const std::string dev_class = cyng::value_cast<std::string>(dom["hardware"].get("class"), "129-129:199.130.83*255");

		//
		//	05 + MAC = server ID
		//
		std::string rnd_mac_str;
		using cyng::io::operator<<;
		std::stringstream ss;
		ss << cyng::generate_random_mac48();
		ss >> rnd_mac_str;
		const std::string mac = cyng::value_cast<std::string>(dom["hardware"].get("mac"), rnd_mac_str);

		std::pair<cyng::mac48, bool > r = cyng::parse_mac48(mac);


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
		//	get wMBus configuration
		//
		cyng::tuple_t cfg_wmbus;
		cfg_wmbus = cyng::value_cast(dom.get("wMBus"), cfg_wmbus);

		/**
		 * global data cache
		 */
		cyng::store::db config_db;
		init_config(logger, config_db, tag, r.first);

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
			, cfg_wmbus
			, cyng::value_cast<std::string>(dom["server"].get("account"), "")
			, cyng::value_cast<std::string>(dom["server"].get("pwd"), "")
			, manufacturer
			, model
			, serial
			, r.first);

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
			, r.first);

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
		, cyng::tuple_t const& cfg_wmbus
		, std::string account
		, std::string pwd
		, std::string manufacturer
		, std::string model
		, std::uint32_t serial
		, cyng::mac48 mac)
	{
		CYNG_LOG_TRACE(logger, "network redundancy: " << cfg_ipt.size());

		cyng::async::start_task_delayed<ipt::network>(mux
			, std::chrono::seconds(1)
			, logger
			, status
			, config_db
			, tag
			, cfg_ipt
			, cfg_wmbus
			, account
			, pwd
			, manufacturer
			, model
			, serial
			, mac);

	}

	void init_config(cyng::logging::log_ptr logger, cyng::store::db& config, boost::uuids::uuid tag, cyng::mac48 mac)
	{
		CYNG_LOG_TRACE(logger, "init configuration db");

		if (!config.create_table(gw_devices()))
		{
			CYNG_LOG_FATAL(logger, "cannot create table devices");
		}

		if (!config.create_table(gw_push_ops()))
		{
			CYNG_LOG_FATAL(logger, "cannot create table push.ops");
		}

		if (!config.create_table(gw_op_log()))
		{
			CYNG_LOG_FATAL(logger, "cannot create table op.log");
		}
	}
}
