/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "listener.h"
#include "mail_config.h"
#include "auth.h"
#include <cyng/log.h>
#include <cyng/async/scheduler.h>
#include <cyng/async/signal_handler.h>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/json.h>
#include <cyng/value_cast.hpp>
#include <cyng/compatibility/io_service.h>

#include <fstream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/filesystem.hpp>
#include <boost/assert.hpp>

namespace node 
{
	//
	//	forward declarations
	//
	bool start(cyng::async::scheduler&, cyng::logging::log_ptr, cyng::object);
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
				cyng::async::scheduler scheduler{this->pool_size_};
				
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
					auto logger = cyng::logging::make_sys_logger("HTTP", true);
// 					auto logger = cyng::logging::make_console_logger(ioc, "HTTP");
#else
					auto logger = cyng::logging::make_console_logger(scheduler.get_io_service(), "HTTP");
#endif
				
					CYNG_LOG_TRACE(logger, cyng::io::to_str(config));
					CYNG_LOG_INFO(logger, "pool size: " << this->pool_size_);

					//
					//	start application
					//
					cyng::vector_t tmp;
					shutdown = start(scheduler, logger, config);
					
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
				//	stop scheduler
				//
				scheduler.stop();
				
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

	int controller::create_config(std::string const& type) const
	{
		return (boost::algorithm::iequals(type, "XML"))
		? create_xml_config()
		: create_json_config()
		;
	}
	
	int controller::create_xml_config() const
	{
		return EXIT_SUCCESS;
	}
	
	int controller::create_json_config() const
	{
		std::fstream fout(json_path_, std::ios::trunc | std::ios::out);
		if (fout.is_open())
		{
			//
			//	get default values
			//
			const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
			const boost::filesystem::path pwd = boost::filesystem::current_path();
			boost::uuids::random_generator rgen;
			
			//
            //  to send emails from cgp see https://cloud.google.com/compute/docs/tutorials/sending-mail/
            //
            
			const auto conf = cyng::vector_factory({
				cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
					, cyng::param_factory("log-level", "INFO")
					, cyng::param_factory("tag", rgen())
					, cyng::param_factory("generated", std::chrono::system_clock::now())
					, cyng::param_factory("favicon", "")
					, cyng::param_factory("mime-config", (pwd / "mime.xml").string())
					, cyng::param_factory("http", cyng::tuple_factory(
						cyng::param_factory("address", "0.0.0.0"),
						cyng::param_factory("service", "8080"),
						cyng::param_factory("document-root", (pwd / "htdocs").string()),
						cyng::param_factory("auth", cyng::vector_factory({
							//	directory: /
							//	authType:
							//	user:
							//	pwd:
							cyng::tuple_factory(
								cyng::param_factory("directory", "/cv"),
								cyng::param_factory("authType", "Basic"),
								cyng::param_factory("realm", "Restricted Content"),
								cyng::param_factory("name", "auth@example.com"),
								cyng::param_factory("pwd", "secret")
							),
							cyng::tuple_factory(
								cyng::param_factory("directory", "/temp"),
								cyng::param_factory("authType", "Basic"),
								cyng::param_factory("realm", "Restricted Content"),
								cyng::param_factory("name", "auth@example.com"),
								cyng::param_factory("pwd", "secret")
							)}
						)),
						cyng::param_factory("redirect", cyng::vector_factory({
							cyng::param_factory("/", "/index.html")
						}))
					))
					, cyng::param_factory("mail", cyng::tuple_factory(
						cyng::param_factory("host", "smtp.gmail.com"),
						cyng::param_factory("port", 465),
 						cyng::param_factory("auth", cyng::tuple_factory(
 							cyng::param_factory("name", "auth@example.com"),
 							cyng::param_factory("pwd", "secret"),
 							cyng::param_factory("method", "START_TLS")
 						)),
 						cyng::param_factory("sender", cyng::tuple_factory(
 							cyng::param_factory("name", "sender"),
 							cyng::param_factory("address", "sender@example.com")
 						)),
 						cyng::param_factory("recipients", cyng::vector_factory({
							cyng::tuple_factory(
								cyng::param_factory("name", "recipient"),
								cyng::param_factory("address", "recipient@example.com"))}
						))
					)))
			});
			
			cyng::json::write(std::cout, cyng::make_object(conf));
			std::cout << std::endl;
			cyng::json::write(fout, cyng::make_object(conf));
			return EXIT_SUCCESS;
		}
		return EXIT_FAILURE;
	}

	/**
	 * Start application - simplified
	 */
	bool start(cyng::async::scheduler& scheduler, cyng::logging::log_ptr logger, cyng::object cfg)
	{
		CYNG_LOG_TRACE(logger, cyng::dom_counter(cfg) << " configuration nodes found" );	
		auto dom = cyng::make_reader(cfg);
		
 		const auto doc_root = cyng::io::to_str(dom[0]["http"].get("document-root"));
		const auto host = cyng::io::to_str(dom[0]["http"].get("address"));
		const auto service = cyng::io::to_str(dom[0]["http"].get("service"));
		const auto port = static_cast<unsigned short>(std::stoi(service));
 		
 		CYNG_LOG_TRACE(logger, "document root: " << doc_root);	

		mail_config mx;
		init(dom[0].get("mail"), mx);

		CYNG_LOG_TRACE(logger, "mx: " << mx);	
		
		auth_dirs ad;
		init(dom[0]["http"].get("auth"), ad);

		
		auto const address = cyng::make_address(host);
		BOOST_ASSERT_MSG(scheduler.is_running(), "scheduler not running");
		
		// Create and launch a listening port
		auto srv = std::make_shared<listener>(
			logger, 
			scheduler.get_io_service(),
			boost::asio::ip::tcp::endpoint{address, port},
			doc_root,
			mx,
 			ad);

		if (srv->run())
		{
			//
			//	wait for system signals
			//
			const bool shutdown = wait(logger);
			
		
//  		std::cerr << "STOP" << std::endl;
		
			//
			//	close acceptor
			//
			CYNG_LOG_INFO(logger, "close acceptor");
			srv->close();
			
			
// 			//
// 			//	stop all connections
// 			//
// 			CYNG_LOG_INFO(logger, "stop all connections");			
			
			return shutdown;
		}
		
		//
		//	shutdown
		//
		return true;
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
