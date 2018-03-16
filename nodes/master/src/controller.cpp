/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "server.h"
#include <NODE_project_info.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/signal_handler.h>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/json.h>
#include <cyng/value_cast.hpp>
#include <fstream>
#include <boost/uuid/random_generator.hpp>
#include <boost/filesystem.hpp>

namespace node 
{
	//
	//	forward declaration(s):
	//
	bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::object);
 	bool wait(cyng::logging::log_ptr logger);
	
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
			bool shutdown {false};
			while (!shutdown)
			{
				//
				//	establish I/O context
				//
				cyng::async::mux mux{this->pool_size_};
				
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
					auto logger = cyng::logging::make_sys_logger("master", true);
// 					auto logger = cyng::logging::make_console_logger(mux.get_io_service(), "master");
#else
					auto logger = cyng::logging::make_console_logger(mux.get_io_service(), "master");
#endif
				
					CYNG_LOG_TRACE(logger, cyng::io::to_str(config));
					
					//
					//	start application
					//
					cyng::vector_t tmp;
					shutdown = start(mux, logger, cyng::value_cast(config, tmp)[0]);
					
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
// 				CYNG_LOG_INFO(logger, "shutdown scheduler");	
				mux.shutdown();
			}
			
			return EXIT_SUCCESS;
		}
		catch (std::exception& ex)
		{
// 			CYNG_LOG_FATAL(logger, ex.what());
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
					, cyng::param_factory("server", cyng::tuple_factory(
						cyng::param_factory("address", "0.0.0.0"),
						cyng::param_factory("service", "7701")
					))
					, cyng::param_factory("session", cyng::tuple_factory(
						cyng::param_factory("connection-open-timeout", 12),		//	seconds - gatekeeper
						cyng::param_factory("connection-close-timeout", 12),	//	seconds
						cyng::param_factory("auto-login", false),
						cyng::param_factory("auto-enabled", true),
						cyng::param_factory("supersede", true)
					))
					, cyng::param_factory("cluster", cyng::tuple_factory(
						cyng::param_factory("account", "root"),
						cyng::param_factory("pwd", NODE_PWD),
						cyng::param_factory("salt", NODE_SALT),
						cyng::param_factory("monitor", 57)	//	seconds
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
	
	bool start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::object cfg)
	{
		CYNG_LOG_TRACE(logger, cyng::dom_counter(cfg) << " configuration nodes found" );		
		auto dom = cyng::make_reader(cfg);

		boost::uuids::random_generator rgen;
		const auto tag = cyng::value_cast<boost::uuids::uuid>(dom.get("tag"), rgen());

		//cyng::param_factory("account", "root"),
		//cyng::param_factory("pwd", NODE_PWD),
		//cyng::param_factory("salt", NODE_SALT),
		//cyng::param_factory("monitor", 57)	//	seconds

		//
		//	create server
		//
		cyng::tuple_t tmp;
		server srv(mux
			, logger
			, tag
			, cyng::value_cast<std::string>(dom["cluster"].get("account"), "")
			, cyng::value_cast<std::string>(dom["cluster"].get("pwd"), "")	//	ToDo: md5 + salt
			, cyng::value_cast(dom["cluster"].get("monitor"), 60)
			, cyng::value_cast(dom.get("session"), tmp));

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
		
}
