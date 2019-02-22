/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/cluster.h"
#include <NODE_project_info.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/async/signal_handler.h>
#include <cyng/factory/set_factory.h>
#include <cyng/set_cast.h>
#include <cyng/io/serializer.h>
#include <cyng/json.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/rnd.h>
#if BOOST_OS_WINDOWS
#include <cyng/scm/service.hpp>
#endif
#if BOOST_OS_LINUX
#include "../../../nodes/write_pid.h"
#endif
#include <cyng/io/iso_3166_1.h>

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/random.hpp>

namespace node 
{
	//
	//	forward declaration(s):
	//
	bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::object);
	bool wait(cyng::logging::log_ptr logger);
	void join_cluster(cyng::async::mux&
		, cyng::logging::log_ptr
		, boost::uuids::uuid
		, std::string language
		, cyng::vector_t const& cfg_cluster
		, cyng::tuple_t cfg_db
		, cyng::tuple_t const& cfg_clock_day
		, cyng::tuple_t const& cfg_clock_hour
		, cyng::tuple_t const& cfg_clock_month
		, cyng::tuple_t const& cfg_trigger);

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

					if (vec.empty()) {
						std::cerr
							<< "use option -D to generate a configuration file"
							<< std::endl;
						shutdown = true;
					}
					else {

						//
						//	initialize logger
						//
#if BOOST_OS_LINUX
						auto logger = cyng::logging::make_sys_logger("task:csv", true);
#else
						const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
						auto dom = cyng::make_reader(vec[0]);
						const boost::filesystem::path log_dir = cyng::value_cast(dom.get("log-dir"), tmp.string());

						auto logger = (console)
							? cyng::logging::make_console_logger(mux.get_io_service(), "task:csv")
							: cyng::logging::make_file_logger(mux.get_io_service(), (log_dir / "task-csv.log"))
							;
#ifdef _DEBUG
						if (!console) {
							std::cout << "log file see: " << (log_dir / "task-csv.log") << std::endl;
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
					// 	CYNG_LOG_FATAL(logger, "no configuration data");
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
			boost::uuids::random_generator uidgen;

			//
			//	generate even distributed integers
			//
			cyng::crypto::rnd_num<int> rng(10, 120);

			const auto conf = cyng::vector_factory({
				cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level", "INFO")
					, cyng::param_factory("tag", uidgen())
					, cyng::param_factory("generated", std::chrono::system_clock::now())
					, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
					, cyng::param_factory("load-config", "local")	//	options are local, master, mixed
					, cyng::param_factory("language", "EN")	//	ISO 2 Letter Language Code (DE, FR, ...)

					, cyng::param_factory("trigger", cyng::tuple_factory(
						cyng::param_factory("offset", 7),	//	minutes after midnight
						cyng::param_factory("frame", 3),	//	time frame in minutes
						cyng::param_factory("format", "SML")	//	supported formats are "SML" and "IEC"
					))

					, cyng::param_factory("profile-15min", cyng::tuple_factory(
						cyng::param_factory("root-dir", (pwd / "csv").string()),
						cyng::param_factory("prefix", "smf-15min-report-"),
                        cyng::param_factory("suffix", "csv"),
						cyng::param_factory("header", true),
						cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
					))

					, cyng::param_factory("profile-60min", cyng::tuple_factory(
						cyng::param_factory("root-dir", (pwd / "csv").string()),
						cyng::param_factory("prefix", "smf-60min-report-"),
                        cyng::param_factory("suffix", "csv"),
						cyng::param_factory("header", true),
						cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
					))
					
					, cyng::param_factory("profile-24h", cyng::tuple_factory(
						cyng::param_factory("root-dir", (pwd / "csv").string()),
						cyng::param_factory("prefix", "smf-24h-report-"),
                        cyng::param_factory("suffix", "csv"),
						cyng::param_factory("header", true),
						cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
					))

					, cyng::param_factory("DB", cyng::tuple_factory(
						cyng::param_factory("type", "SQLite"),
						cyng::param_factory("file-name", (pwd / "store.database").string()),
						cyng::param_factory("busy-timeout", 12),		//	seconds
						cyng::param_factory("watchdog", 30),	//	for database connection
						cyng::param_factory("pool-size", 1),	//	no pooling for SQLite
						cyng::param_factory("db-schema", NODE_SUFFIX),		//	use "v4.0" for compatibility to version 4.x
						cyng::param_factory("period", 12)	//	seconds
					))

					, cyng::param_factory("cluster", cyng::vector_factory({ cyng::tuple_factory(
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "7701"),
						cyng::param_factory("account", "root"),
						cyng::param_factory("pwd", NODE_PWD),
						cyng::param_factory("salt", NODE_SALT),
						cyng::param_factory("monitor", rng()),	//	seconds
						cyng::param_factory("group", 0)	//	customer ID
					) }))

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
			::OutputDebugString("An instance of the [csv] service is already running.");
			break;
		case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
			//
			//	The service process could not connect to the service controller.
			//	Typical error message, when running in console mode.
			//
			::OutputDebugString("***Error 1063: The [csv] service process could not connect to the service controller.");
			std::cerr
				<< "***Error 1063: The [csv] service could not connect to the service controller."
				<< std::endl
				;
			break;
		case ERROR_SERVICE_NOT_IN_EXE:
			//	The executable program that this service is configured to run in does not implement the service.
			::OutputDebugString("The [csv] service is configured to run in does not implement the service.");
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

		boost::uuids::random_generator uidgen;
		auto const cluster_tag = cyng::value_cast<boost::uuids::uuid>(dom.get("tag"), uidgen());
		CYNG_LOG_INFO(logger, "cluster tag: " << cluster_tag);

		auto const language = cyng::value_cast<std::string>(dom.get("language"), "EN");
		std::uint32_t cc = cyng::io::CC_GB;
		if (language.size() == 2) {
			CYNG_LOG_INFO(logger, "language code: " << language);
			cc = cyng::io::get_code(language.at(0), language.at(1));
		}
		else {
			CYNG_LOG_WARNING(logger, "invalid language code: " << language);
		}

		//
		//	apply severity threshold
		//
		logger->set_severity(cyng::logging::to_severity(cyng::value_cast<std::string>(dom.get("log-level"), "INFO")));

#if BOOST_OS_LINUX
        const boost::filesystem::path log_dir = cyng::value_cast<std::string>(dom.get("log-dir"), ".");
		write_pid(log_dir, cluster_tag);
#endif

		//
		//	storage manager task
		//	Open database session only when needed (from task clock)
		//
		cyng::tuple_t tpl;

		//
		//	connect to cluster
		//
		cyng::vector_t vec;
		join_cluster(mux
			, logger
			, cluster_tag
			, language
			, cyng::value_cast(dom.get("cluster"), vec)
			, cyng::value_cast(dom.get("DB"), tpl)
			, cyng::value_cast(dom.get("profile-15min"), tpl)
			, cyng::value_cast(dom.get("profile-60min"), tpl)
			, cyng::value_cast(dom.get("profile-24h"), tpl)
			, cyng::value_cast(dom.get("trigger"), tpl));

		//
		//	wait for system signals
		//
		const bool shutdown = wait(logger);

		//
		//	close acceptor
		//
		CYNG_LOG_INFO(logger, "close acceptor");

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

	void join_cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, std::string language
		, cyng::vector_t const& cfg_cluster
		, cyng::tuple_t cfg_db
		, cyng::tuple_t const& cfg_clock_day
		, cyng::tuple_t const& cfg_clock_hour
		, cyng::tuple_t const& cfg_clock_month
		, cyng::tuple_t const& cfg_trigger)
	{
		CYNG_LOG_TRACE(logger, "cluster redundancy: " << cfg_cluster.size());

		cyng::async::start_task_delayed<cluster>(mux
			, std::chrono::seconds(1)
			, logger
			, tag
			, language
			, load_cluster_cfg(cfg_cluster)
			, cyng::to_param_map(cfg_db)
			, cyng::to_param_map(cfg_clock_day)
			, cyng::to_param_map(cfg_clock_hour)
			, cyng::to_param_map(cfg_clock_month)
			, cyng::to_param_map(cfg_trigger));
	}
}
