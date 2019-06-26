/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include <NODE_project_info.h>
#include <smf/http/srv/mail_config.h>
#include "logic.h"
#include <smf/http/srv/auth.h>
#include <smf/http/srv/server.h>


#include <cyng/log.h>
#include <cyng/async/scheduler.h>
#include <cyng/async/signal_handler.h>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/json.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/vector_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/compatibility/io_service.h>
#include <cyng/vm/controller.h>

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
						auto logger = cyng::logging::make_sys_logger("HTTP", true);
						// 	auto logger = cyng::logging::make_console_logger(ioc, "HTTP");
#else
						auto logger = cyng::logging::make_console_logger(scheduler.get_io_service(), "HTTP");
#endif

						CYNG_LOG_TRACE(logger, cyng::io::to_str(config));
						CYNG_LOG_INFO(logger, "pool size: " << this->pool_size_);

						//
						//	start application
						//
						shutdown = start(scheduler, logger, vec.at(0));

						//
						//	print uptime
						//
						const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tp_start);
						CYNG_LOG_INFO(logger, "HTTP uptime " << cyng::io::to_str(cyng::make_object(duration)));
					}
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
			boost::uuids::random_generator uidgen;
			
			//
            //  to send emails from cgp see https://cloud.google.com/compute/docs/tutorials/sending-mail/
            //
            
			const auto conf = cyng::vector_factory({
				cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
					, cyng::param_factory("log-level", "INFO")
					, cyng::param_factory("tag", uidgen())
					, cyng::param_factory("generated", std::chrono::system_clock::now())
					, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
					, cyng::param_factory("http", cyng::tuple_factory(
						cyng::param_factory("address", "0.0.0.0"),
						cyng::param_factory("service", "8080"),
						cyng::param_factory("timeout", "15"),	//	seconds
#if BOOST_OS_LINUX
						cyng::param_factory("document-root", "/var/www/html"),
						cyng::param_factory("blog-root", "/var/www/html/blog"),
#else
						cyng::param_factory("document-root", (pwd / "htdocs").string()),
						cyng::param_factory("blog-root", (pwd / "htdocs" / "blog").string()),
#endif
						cyng::param_factory("auth", cyng::vector_factory({
							//	directory: /
							//	authType:
							//	user:
							//	pwd:
							cyng::tuple_factory(
								cyng::param_factory("directory", "/"),
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
						})),
						cyng::param_factory("https-rewrite", false),	//	301 - Moved Permanently
						cyng::param_factory("blacklist", cyng::vector_factory({
							//	https://bl.isx.fr/raw
							cyng::make_address("185.244.25.187"),	//	KV Solutions B.V. scans for "login.cgi"
							cyng::make_address("139.219.100.104"),	//	ISP Microsoft (China) Co. Ltd. - 2018-07-31T21:14
							cyng::make_address("194.147.32.109"),	//	Udovikhin Evgenii - 2019-02-01 15:23:08.13699453
							cyng::make_address("185.209.0.12"),		//	2019-03-27 11:23:39
							cyng::make_address("42.236.101.234"),	//	hn.kd.ny.adsl (china)
							cyng::make_address("185.104.184.126"),	//	M247 Ltd
							cyng::make_address("185.162.235.56")	//	SILEX malware
						}))	//	blacklist
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
					))
				)
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
		
 		auto const doc_root = cyng::io::to_str(dom["http"].get("document-root"));
		auto const blog_root = cyng::io::to_str(dom["http"].get("blog-root"));
		auto const host = cyng::io::to_str(dom["http"].get("address"));
		auto const service = cyng::io::to_str(dom["http"].get("service"));
		auto const port = static_cast<unsigned short>(std::stoi(service));
		auto const timeout = cyng::numeric_cast<std::size_t>(dom["http"].get("timeout"), 15u);

 		CYNG_LOG_TRACE(logger, "document root: " << doc_root);	
		CYNG_LOG_TRACE(logger, "blog root: " << blog_root);
		CYNG_LOG_INFO(logger, "HTTP address: " << host);
		CYNG_LOG_INFO(logger, "HTTP service: " << port);
		CYNG_LOG_INFO(logger, "HTTP timeout: " << timeout << " seconds");

		auto const https_rewrite = cyng::value_cast(dom["http"].get("https-rewrite"), false);
		if (https_rewrite) {
			CYNG_LOG_WARNING(logger, "HTTPS rewrite is active");
		}
		
		mail_config mx;
		init(dom.get("mail"), mx);

		CYNG_LOG_TRACE(logger, "mx: " << mx);	
		
		auth_dirs ad;
		init(dom["http"].get("auth"), ad);

		
		auto const address = cyng::make_address(host);

		//
		//	get blacklisted addresses
		//
		const auto blacklist_str = cyng::vector_cast<std::string>(dom["http"].get("blacklist"), "");
		CYNG_LOG_INFO(logger, blacklist_str.size() << " adresses are blacklisted");
		std::set<boost::asio::ip::address>	blacklist;
		for (auto const& a : blacklist_str) {
			auto r = blacklist.insert(boost::asio::ip::make_address(a));
			if (r.second) {
				CYNG_LOG_TRACE(logger, *r.first);
			}
			else {
				CYNG_LOG_WARNING(logger, "cannot insert " << a);
			}
		}

		//
		//	redirects
		//
		cyng::vector_t vec;
		auto const rv = cyng::value_cast(dom.get("redirect"), vec);
		auto const rs = cyng::to_param_map(rv);	// cyng::param_map_t
		CYNG_LOG_INFO(logger, rs.size() << " redirects configured");
		std::map<std::string, std::string> redirects;
		for (auto const& redirect : rs) {
			auto const target = cyng::value_cast<std::string>(redirect.second, "");
			CYNG_LOG_TRACE(logger, redirect.first
				<< " ==> "
				<< target);
			redirects.emplace(redirect.first, target);
		}

		//
		//	create VM controller
		//
		BOOST_ASSERT_MSG(scheduler.is_running(), "scheduler not running");
		boost::uuids::random_generator uidgen;
		cyng::controller vm(scheduler.get_io_service(), uidgen(), std::cout, std::cerr);

		// Create and launch a listening port
		auto srv = std::make_shared<http::server>(logger
			, scheduler.get_io_service()
			, boost::asio::ip::tcp::endpoint{ address, port }
			, timeout
			, doc_root
			, blog_root
#ifdef NODE_SSL_INSTALLED
			, ad
#endif
			, blacklist
			, redirects
			, vm
			, https_rewrite);


		//
		//	add logic
		//
		http::logic handler(*srv, vm, logger);

		if (srv->run())
		{
			//
			//	wait for system signals
			//
			const bool shutdown = wait(logger);
			
				
			//
			//	close acceptor
			//
			CYNG_LOG_INFO(logger, "close acceptor");
			srv->close();
					
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
