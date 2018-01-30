/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include <NODE_project_info.h>
#include "tasks/storage_db.h"
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
#include <fstream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/filesystem.hpp>

namespace node 
{
	//
	//	forward declaration(s):
	//
	bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::object);
	bool wait(cyng::logging::log_ptr logger);
	void join_network(cyng::async::mux&, cyng::logging::log_ptr, std::vector<std::size_t> const&, cyng::vector_t const&, std::vector<std::string> const&);
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
					//	initialize logger
					//
#if BOOST_OS_LINUX
					auto logger = cyng::logging::make_sys_logger("store", true);
#else
					auto logger = cyng::logging::make_console_logger(mux.get_io_service(), "store");
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

			const auto conf = cyng::vector_factory({
				cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", rgen())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
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
					, cyng::param_factory("targets", cyng::vector_factory({ "data.sink.1", "data.sink.2" }))	//	list of targets
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
			return ipt::storage_db::init_db(cyng::value_cast(dom.get("DB"), tpl));
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
		//const auto config_type = cyng::io::to_str(dom.get("output"));
		const auto config_types = cyng::vector_cast<std::string>(dom.get("output"), "");
		//CYNG_LOG_TRACE(logger, "config types are " << config_type);

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
		//	connect to cluster
		//
		cyng::vector_t tmp;
		join_network(mux, logger, tsks, cyng::value_cast(dom.get("ipt"), tmp), cyng::vector_cast<std::string>(dom.get("targets"), ""));

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

				tsks.push_back(cyng::async::start_task_delayed<ipt::storage_db>(mux
					, std::chrono::seconds(1)
					, logger
					, cyng::to_param_map(tpl)).first);
			}
			//else if (boost::algorithm::iequals(config_type, "XML"))
			//{
			//	CYNG_LOG_INFO(logger, "open XML configuration files");

			//	return cyng::async::start_task_delayed<ipt::storage_xml>(mux
			//		, std::chrono::seconds(1)
			//		, logger
			//		, cyng::to_param_map(cfg)).first;
			//}
			//else if (boost::algorithm::iequals(config_type, "JSON"))
			//{
			//	CYNG_LOG_INFO(logger, "open JSONL configuration files");

			//	return cyng::async::start_task_delayed<ipt::storage_json>(mux
			//		, std::chrono::seconds(1)
			//		, logger
			//		, cyng::to_param_map(cfg)).first;
			//}
			else
			{
				CYNG_LOG_ERROR(logger, "unknown config type " << config_type);
			}
		}
		return tsks;
	}

	void join_network(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, std::vector<std::size_t> const& tsks
		, cyng::vector_t const& cfg
		, std::vector<std::string> const& targets)
	{
		CYNG_LOG_TRACE(logger, "network redundancy: " << cfg.size());

		cyng::async::start_task_delayed<ipt::network>(mux
			, std::chrono::seconds(1)
			, logger
			, tsks
			, ipt::load_cluster_cfg(cfg)
			, targets);

	}

}
