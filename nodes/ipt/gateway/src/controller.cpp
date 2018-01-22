/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
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
	void join_network(cyng::async::mux&, cyng::logging::log_ptr, cyng::vector_t const&);
	//std::vector<std::size_t> connect_data_store(cyng::async::mux&, cyng::logging::log_ptr, std::vector<std::string> const&, cyng::object cfg);

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

			const auto conf = cyng::vector_factory({
				cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", rgen())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				//, cyng::param_factory("output", cyng::vector_factory({ "DB" }))	//	options are XML, JSON, DB

				//	on this address the gateway acts as a server
				, cyng::param_factory("server", cyng::tuple_factory(
					cyng::param_factory("address", "0.0.0.0"),
					cyng::param_factory("service", "7259"),
					cyng::param_factory("discover", "5798"),	//	UDP
					cyng::param_factory("account", "operator"),
					cyng::param_factory("pwd", "operator")
				))
				, cyng::param_factory("hardware", cyng::tuple_factory(
					cyng::param_factory("manufacturer", "solosTec"),	//	manufacturer (81 81 C7 82 03 FF)
					cyng::param_factory("model", "Gateway"),	//	Typenschlüssel (81 81 C7 82 09 FF --> 81 81 C7 82 0A 01)
					cyng::param_factory("serial", rgen()),	//	Seriennummer (81 81 C7 82 09 FF --> 81 81 C7 82 0A 02)
					cyng::param_factory("class", "129-129:199.130.83*255")	//	device class (81 81 C7 82 02 FF) MUC-LAN/DSL
					//cyng::param_factory("mac", "")	//	mac 
				))
				, cyng::param_factory("ipt", cyng::vector_factory({
					cyng::tuple_factory(
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "26862"),
						cyng::param_factory("account", "smart-data-exchange"),
						cyng::param_factory("pwd", "to-define"),
						cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::param_factory("scrambled", true),
						cyng::param_factory("monitor", 57)),	//	seconds
					cyng::tuple_factory(
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "26863"),
						cyng::param_factory("account", "smart-data-exchange"),
						cyng::param_factory("pwd", "to-define"),
						cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::param_factory("scrambled", false),
						cyng::param_factory("monitor", 57))
					}))
					//, cyng::param_factory("targets", cyng::vector_factory({ "data.sink.1", "data.sink.2" }))	//	list of targets
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


	bool start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::object cfg)
	{
		CYNG_LOG_TRACE(logger, cyng::dom_counter(cfg) << " configuration nodes found");
		cyng::select_reader<cyng::object>::type dom(cfg);

		boost::uuids::random_generator rgen;
		const auto tag = cyng::value_cast<boost::uuids::uuid>(dom.get("tag"), rgen());

		//
		//	get configuration type
		//
		//const auto config_types = cyng::vector_cast<std::string>(dom.get("output"), "");


		//
		//	connect to cluster
		//
		cyng::vector_t tmp;
		join_network(mux, logger, cyng::value_cast(dom.get("ipt"), tmp));

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


	void join_network(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::vector_t const& cfg)
	{
		CYNG_LOG_TRACE(logger, "network redundancy: " << cfg.size());

		cyng::async::start_task_delayed<ipt::network>(mux
			, std::chrono::seconds(1)
			, logger
			, ipt::load_cluster_cfg(cfg));

	}
}
