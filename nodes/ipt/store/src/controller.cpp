/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include <NODE_project_info.h>
#include "tasks/storage_db.h"
#include "tasks/storage_xml.h"
#include "tasks/storage_abl.h"
#include "tasks/storage_binary.h"
#include "tasks/storage_json.h"
#include "tasks/storage_log.h"
#include "tasks/network.h"
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/async/signal_handler.h>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/json.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/vector_cast.hpp>
#if BOOST_OS_WINDOWS
#include <cyng/scm/service.hpp>
#endif
#if BOOST_OS_LINUX
#include "../../../write_pid.h"
#endif
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

namespace node 
{
	//
	//	forward declaration(s):
	//
	bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::object);
	bool wait(cyng::logging::log_ptr logger);
	void join_network(cyng::async::mux&, cyng::logging::log_ptr, std::vector<std::size_t> const&, cyng::vector_t const&, cyng::param_map_t const&);
	std::vector<std::size_t> connect_data_store(cyng::async::mux&, cyng::logging::log_ptr, std::vector<std::string> const&, cyng::object cfg);

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
						shutdown = true;

						std::cerr
							<< "use option -D to generate a configuration file"
							<< std::endl;
					}
					else
					{

						//
						//	initialize logger
						//
#if BOOST_OS_LINUX
						auto logger = cyng::logging::make_sys_logger("ipt:master", true);
#else
						const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
						auto dom = cyng::make_reader(vec[0]);
						const boost::filesystem::path log_dir = cyng::value_cast(dom.get("log-dir"), tmp.string());

						auto logger = (console)
							? cyng::logging::make_console_logger(mux.get_io_service(), "ipt:master")
							: cyng::logging::make_file_logger(mux.get_io_service(), (log_dir / "ipt-master.log"))
							;
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
			boost::uuids::random_generator rgen;

			const auto conf = cyng::vector_factory({
				cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", rgen())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("log-pushdata", false)	//	log file for each channel
				, cyng::param_factory("output", cyng::vector_factory({"DB"}))	//	options are XML, JSON, DB

				, cyng::param_factory("DB", cyng::tuple_factory(
					cyng::param_factory("type", "SQLite"),
					cyng::param_factory("file-name", (pwd / "store.database").string()),
					cyng::param_factory("busy-timeout", 12),		//	seconds
					cyng::param_factory("watchdog", 30),		//	for database connection
					cyng::param_factory("pool-size", 1)		//	no pooling for SQLite
				))
				, cyng::param_factory("XML", cyng::tuple_factory(
					cyng::param_factory("root-dir", (pwd / "xml").string()),
					cyng::param_factory("root-name", "SML"),
					cyng::param_factory("endcoding", "UTF-8")
				))
				, cyng::param_factory("JSON", cyng::tuple_factory(
					cyng::param_factory("root-dir", (pwd / "json").string())
				))
				, cyng::param_factory("ABL", cyng::tuple_factory(
					cyng::param_factory("root-dir", (pwd / "abl").string()),
					cyng::param_factory("prefix", "smf")
				))
				, cyng::param_factory("BIN", cyng::tuple_factory(
					cyng::param_factory("root-dir", (pwd / "sml").string()),
					cyng::param_factory("prefix", "smf"),
					cyng::param_factory("suffix", "sml")
				))
				, cyng::param_factory("LOG", cyng::tuple_factory(
					cyng::param_factory("root-dir", (pwd / "log").string()),
					cyng::param_factory("prefix", "smf")
				))
				, cyng::param_factory("ipt", cyng::vector_factory({
					cyng::tuple_factory(
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "26862"),
						cyng::param_factory("account", "data-store"),
						cyng::param_factory("pwd", "to-define"),
						cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::param_factory("scrambled", true),
						cyng::param_factory("monitor", 57)),	//	seconds
					cyng::tuple_factory(
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "26863"),
						cyng::param_factory("account", "data-store"),
						cyng::param_factory("pwd", "to-define"),
						cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::param_factory("scrambled", false),
						cyng::param_factory("monitor", 57))
					}))
					//, cyng::param_factory("targets", cyng::vector_factory({ "data.sink.1", "data.sink.2" }))	//	list of targets
					, cyng::param_factory("targets", cyng::vector_factory({ 
						  cyng::param_factory("data.sink.sml", "SML")
						, cyng::param_factory("data.sink.iec", "IEC") //	IEC 62056-21
						}))	//	list of targets
				)
				});
			//cyng::vector_factory({});

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

	int controller::init_db()
	{
		//
		//	read configuration file
		//
		cyng::object config = cyng::json::read_file(json_path_);

		cyng::vector_t vec;
		vec = cyng::value_cast(config, vec);
		BOOST_ASSERT_MSG(!vec.empty(), "invalid configuration");

		if (!vec.empty())
		{
			auto dom = cyng::make_reader(vec[0]);
			cyng::tuple_t tpl;
			return storage_db::init_db(cyng::value_cast(dom.get("DB"), tpl));
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
			::OutputDebugString("An instance of the [ipt:store] service is already running.");
			break;
		case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
			//
			//	The service process could not connect to the service controller.
			//	Typical error message, when running in console mode.
			//
			::OutputDebugString("***Error 1063: The [ipt:store] service process could not connect to the service controller.");
			std::cerr
				<< "***Error 1063: The [ipt:store] service could not connect to the service controller."
				<< std::endl
				;
			break;
		case ERROR_SERVICE_NOT_IN_EXE:
			//	The executable program that this service is configured to run in does not implement the service.
			::OutputDebugString("The [ipt:store] service is configured to run in does not implement the service.");
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

		boost::uuids::random_generator rgen;
		const auto tag = cyng::value_cast<boost::uuids::uuid>(dom.get("tag"), rgen());

		//
		//	apply severity threshold
		//
		logger->set_severity(cyng::logging::to_severity(cyng::value_cast<std::string>(dom.get("log-level"), "INFO")));

        //
        //  control logging of push data
        //
		const auto log_pushdata = cyng::value_cast(dom.get("log-pushdata"), false);
		
#if BOOST_OS_LINUX
        const boost::filesystem::path log_dir = cyng::value_cast<std::string>(dom.get("log-dir"), ".");
        write_pid(log_dir, tag);
#endif

		//
		//	get configuration type
		//
		const auto config_types = cyng::vector_cast<std::string>(dom.get("output"), "");

		//
		//	storage manager task
		//
		cyng::tuple_t tpl;
		auto tsks = connect_data_store(mux, logger, config_types, cfg);
		if (tsks.empty())
		{
			CYNG_LOG_FATAL(logger, "no output channels found");
			return true;	//	shutdown
		}
		CYNG_LOG_TRACE(logger, tsks.size() << " output channel(s) defined");

		//
		//	connect to ipt master
		//
		cyng::vector_t tmp;
		//	vector of tuples with parameters
		cyng::vector_t target_vec;
		target_vec = cyng::value_cast(dom.get("targets"), target_vec);

		join_network(mux
			, logger
			, tsks
			, cyng::value_cast(dom.get("ipt"), tmp)
			, cyng::to_param_map(target_vec));

		//
		//	wait for system signals
		//
		const bool shutdown = wait(logger);

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

	std::vector<std::size_t> connect_data_store(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, std::vector<std::string> const& config_types
		, cyng::object cfg)
	{
		std::vector<std::size_t> tsks;
		cyng::tuple_t tpl;
		auto dom = cyng::make_reader(cfg);

		for (const auto& config_type : config_types)
		{
			if (boost::algorithm::iequals(config_type, "DB"))
			{
				CYNG_LOG_INFO(logger, "connect to configuration database");

				tpl = cyng::value_cast(dom.get(config_type), tpl);

				tsks.push_back(cyng::async::start_task_delayed<storage_db>(mux
					, std::chrono::seconds(1)
					, logger
					, cyng::to_param_map(tpl)).first);
			}
			else if (boost::algorithm::iequals(config_type, "XML"))
			{
				CYNG_LOG_INFO(logger, "start XML storage");

				tpl = cyng::value_cast(dom.get(config_type), tpl);

				auto root_dir = cyng::value_cast<std::string>(dom[config_type].get("root-dir"), "");
				auto root_name = cyng::value_cast<std::string>(dom[config_type].get("root-name"), "SML");
				auto encoding = cyng::value_cast<std::string>(dom[config_type].get("endcoding"), "UTF-8");

				tsks.push_back(cyng::async::start_task_delayed<storage_xml>(mux
					, std::chrono::seconds(1)
					, logger
					, root_dir
					, root_name
					, encoding).first);
			}
			else if (boost::algorithm::iequals(config_type, "JSON"))
			{
				CYNG_LOG_INFO(logger, "start JSON storage");

				tpl = cyng::value_cast(dom.get(config_type), tpl);

				auto root_dir = cyng::value_cast<std::string>(dom[config_type].get("root-dir"), "");
				auto prefix = cyng::value_cast<std::string>(dom[config_type].get("prefix"), "sml");
				auto suffix = cyng::value_cast<std::string>(dom[config_type].get("suffix"), "json");

				tsks.push_back(cyng::async::start_task_delayed<storage_json>(mux
					, std::chrono::seconds(1)
					, logger
					, root_dir
					, prefix
					, suffix).first);
			}
			else if (boost::algorithm::iequals(config_type, "ABL"))
			{
				CYNG_LOG_INFO(logger, "start ABL storage");

				tpl = cyng::value_cast(dom.get(config_type), tpl);

				auto root_dir = cyng::value_cast<std::string>(dom[config_type].get("root-dir"), "");
				auto prefix = cyng::value_cast<std::string>(dom[config_type].get("prefix"), "sml");
				auto suffix = cyng::value_cast<std::string>(dom[config_type].get("suffix"), "abl");

				tsks.push_back(cyng::async::start_task_delayed<storage_abl>(mux
					, std::chrono::seconds(1)
					, logger
					, cyng::to_param_map(tpl)).first);
			}
			else if (boost::algorithm::iequals(config_type, "BIN"))
			{
				CYNG_LOG_INFO(logger, "start BINary storage");

				auto root_dir = cyng::value_cast<std::string>(dom[config_type].get("root-dir"), "");
				auto prefix = cyng::value_cast<std::string>(dom[config_type].get("prefix"), "sml");
				auto suffix = cyng::value_cast<std::string>(dom[config_type].get("suffix"), "sml");

				tsks.push_back(cyng::async::start_task_delayed<storage_binary>(mux
					, std::chrono::seconds(1)
					, logger
					, root_dir
					, prefix
					, suffix).first);
			}
			else if (boost::algorithm::iequals(config_type, "LOG"))
			{
				CYNG_LOG_INFO(logger, "start LOG storage");

				tpl = cyng::value_cast(dom.get(config_type), tpl);

				tsks.push_back(cyng::async::start_task_delayed<storage_log>(mux
					, std::chrono::seconds(1)
					, logger
					, cyng::to_param_map(tpl)).first);
			}
			else
			{
				CYNG_LOG_ERROR(logger, "unknown config type " << config_type);
			}

			tpl.clear();
		}
		return tsks;
	}

	void join_network(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, std::vector<std::size_t> const& tsks
		, cyng::vector_t const& cfg
		, cyng::param_map_t const& targets)
	{
		CYNG_LOG_TRACE(logger, cfg.size() << " IP-T master(s) configured");
		CYNG_LOG_TRACE(logger, targets.size() << " push target(s) configured:");

		std::map<std::string, std::string>	target_cfg;
		for (auto const& target : targets)
		{
			auto pos = target_cfg.emplace(target.first, cyng::value_cast<std::string>(target.second, "SML"));
			CYNG_LOG_TRACE(logger, pos->first << ':' << pos->second);
			if (!boost::algorithm::equals(pos->second, "SML")) {
				CYNG_LOG_WARNING(logger, "data layer " << pos->second << " not supported");
			}
		}

		cyng::async::start_task_delayed<ipt::network>(mux
			, std::chrono::seconds(1)
			, logger
			, tsks
			, ipt::load_cluster_cfg(cfg)
			, target_cfg);

	}

}
