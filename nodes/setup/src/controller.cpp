/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/cluster.h"
#include "tasks/storage_db.h"
#include "tasks/storage_json.h"
#include "tasks/storage_xml.h"
#include <NODE_project_info.h>
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
#include <fstream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/filesystem.hpp>
#include <boost/random.hpp>

namespace node 
{
	//
	//	forward declaration(s):
	//
	bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::object);
	bool wait(cyng::logging::log_ptr logger);
	void join_cluster(cyng::async::mux&, cyng::logging::log_ptr, cyng::store::db&, std::size_t, cyng::vector_t const&);
	std::size_t connect_data_store(cyng::async::mux&, cyng::logging::log_ptr, cyng::store::db&, std::string, cyng::tuple_t);

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
					//	initialize logger
					//
#if BOOST_OS_LINUX
					auto logger = cyng::logging::make_sys_logger("setup", true);
#else
					auto logger = cyng::logging::make_console_logger(mux.get_io_service(), "setup");
#endif

					CYNG_LOG_TRACE(logger, cyng::io::to_str(config));

					//
					//	start application
					//
					cyng::vector_t vec;
					vec = cyng::value_cast(config, vec);
					BOOST_ASSERT_MSG(!vec.empty(), "invalid configuration");
					shutdown = vec.empty()
						? true
						: start(mux, logger, vec[0]);

					//
					//	print uptime
					//
					const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tp_start);
					CYNG_LOG_INFO(logger, "uptime " << cyng::io::to_str(cyng::make_object(duration)));
				}
				else
				{
					// 					CYNG_LOG_FATAL(logger, "no configuration data");
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

			boost::random::mt19937 rng_;
			boost::random::uniform_int_distribution<int> monitor_dist(10, 120);

			const auto conf = cyng::vector_factory({
				cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", rgen())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("input", "DB")	//	options are XML, JSON, DB

				, cyng::param_factory("DB", cyng::tuple_factory(
					cyng::param_factory("type", "SQLite"),
					cyng::param_factory("file-name", (pwd / "setup.database").string()),
					cyng::param_factory("busy-timeout", 12),		//	seconds
					cyng::param_factory("watchdog", 30),		//	for database connection
					cyng::param_factory("pool-size", 1)		//	no pooling for SQLite
				))
				, cyng::param_factory("XML", cyng::tuple_factory(
					cyng::param_factory("file-name", (pwd / "setup.config.xml").string()),
					cyng::param_factory("endcoding", "UTF-8")
				))
				, cyng::param_factory("JSON", cyng::tuple_factory(
					cyng::param_factory("file-name", (pwd / "setup.config.json").string())
				))
				, cyng::param_factory("cluster", cyng::vector_factory({ cyng::tuple_factory(
					cyng::param_factory("host", "127.0.0.1"),
					cyng::param_factory("service", "7701"),
					cyng::param_factory("account", "root"),
					cyng::param_factory("pwd", NODE_PWD),
					cyng::param_factory("salt", NODE_SALT),
					cyng::param_factory("monitor", monitor_dist(rng_))	//	seconds
				)})	)
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

	bool start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::object cfg)
	{
		CYNG_LOG_TRACE(logger, cyng::dom_counter(cfg) << " configuration nodes found");
		auto dom = cyng::make_reader(cfg);

		boost::uuids::random_generator rgen;
		const auto tag = cyng::value_cast<boost::uuids::uuid>(dom.get("tag"), rgen());

		//
		//	get configuration type
		//
		const auto config_type = cyng::io::to_str(dom.get("input"));
		CYNG_LOG_TRACE(logger, "config type is " << config_type);

		/**
		 * global data cache
		 */
		cyng::store::db cache_;

		//
		//	storage manager task
		//
		cyng::tuple_t tpl;
		std::size_t storage_task = connect_data_store(mux, logger, cache_, config_type, cyng::value_cast(dom.get(config_type), tpl));

		//
		//	connect to cluster
		//
		cyng::vector_t tmp;
		join_cluster(mux, logger, cache_, storage_task, cyng::value_cast(dom.get("cluster"), tmp));

		//
		//	wait for system signals
		//
		const bool shutdown = wait(logger);

		//
		//	close acceptor
		//
		CYNG_LOG_INFO(logger, "close acceptor");
		//srv.close();

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
		, cyng::store::db& cache
		, std::size_t tsk
		, cyng::vector_t const& cfg)
	{
		CYNG_LOG_TRACE(logger, "cluster redundancy: " << cfg.size());

		cyng::async::start_task_delayed<cluster>(mux
			, std::chrono::seconds(1)
			, logger
			, cache
			, tsk
			, load_cluster_cfg(cfg));

	}

	std::size_t connect_data_store(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cyng::store::db& cache
		, std::string config_type
		, cyng::tuple_t cfg)
	{
		if (boost::algorithm::iequals(config_type, "DB"))
		{
			CYNG_LOG_INFO(logger, "connect to configuration database");

			return cyng::async::start_task_delayed<storage_db>(mux
				, std::chrono::seconds(1)
				, logger
				, cache
				, cyng::to_param_map(cfg)).first;
		}
		else if (boost::algorithm::iequals(config_type, "XML"))
		{
			CYNG_LOG_INFO(logger, "open XML configuration files");

			return cyng::async::start_task_delayed<storage_xml>(mux
				, std::chrono::seconds(1)
				, logger
				, cache
				, cyng::to_param_map(cfg)).first;
		}
		else if (boost::algorithm::iequals(config_type, "JSON"))
		{
			CYNG_LOG_INFO(logger, "open JSONL configuration files");

			return cyng::async::start_task_delayed<storage_json>(mux
				, std::chrono::seconds(1)
				, logger
				, cache
				, cyng::to_param_map(cfg)).first;
		}
		CYNG_LOG_FATAL(logger, "unknown config type " << config_type);
		return cyng::async::NO_TASK;
	}

}
