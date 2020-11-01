/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "server.h"
#include <NODE_project_info.h>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/value_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fstream>

namespace node 
{
	controller::controller(unsigned int index
		, unsigned int pool_size
		, std::string const& json_path
		, std::string node_name)
	: ctl(index
			, pool_size
			, json_path
			, node_name)
	{}

	cyng::vector_t controller::create_config(std::fstream& fout, cyng::filesystem::path&& tmp, cyng::filesystem::path&& cwd) const
	{	
		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level",
#ifdef _DEBUG
					"TRACE"
#else
					"INFO"
#endif
				)
				, cyng::param_factory("tag", get_random_tag())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
				, cyng::param_factory("country-code", "CH")
				, cyng::param_factory("language-code", "EN")

				, cyng::param_factory("server", cyng::tuple_factory(
					cyng::param_factory("address", "0.0.0.0"),
					cyng::param_factory("service", "7701")
				))
				, cyng::param_factory("session", cyng::tuple_factory(
					cyng::param_factory("auto-login", false),
					cyng::param_factory("auto-enabled", true),
					cyng::param_factory("supersede", true),
					cyng::param_factory("gw-cache", true),
					cyng::param_factory("generate-time-series", false),
					cyng::param_factory("catch-meters", false),
					cyng::param_factory("catch-lora", true),
					cyng::param_factory("stat-dir", tmp.string()),	//	store statistics
					cyng::param_factory("max-messages", 1000),	//	system log
					cyng::param_factory("max-events", 2000),		//	time series events
					cyng::param_factory("max-LoRa-records", 500),		//	LoRa uplink records
					cyng::param_factory("max-wMBus-records", 500)		//	wireless M-Bus uplink records
					//cyng::param_factory("auto-gw", true)	//	insert gateways automatically
				))
				, cyng::param_factory("cluster", cyng::tuple_factory(
					cyng::param_factory("account", "root"),
					cyng::param_factory("pwd", NODE_PWD),
					cyng::param_factory("salt", NODE_SALT),
					cyng::param_factory("monitor", 57)	//	seconds
				))
			)
		});
	}
	
	bool controller::start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag)
	{
		auto const country_code = cyng::value_cast<std::string>(cfg.get("country-code"), "CH");
		auto const language_code = cyng::value_cast<std::string>(cfg.get("language-code"), "EN");

		//
		//	create server
		//
		cyng::tuple_t tmp;
        tmp = cyng::value_cast(cfg.get("session"), tmp);

        //CYNG_LOG_TRACE(logger, tmp.size()
        //    << '/'
        //    << cyng::dom_counter(cfg)
        //    << " configuration nodes (session/total)" );

        server srv(mux
			, logger
			, tag
			, country_code
			, language_code
			, cyng::value_cast<std::string>(cfg["cluster"].get("account"), "")
			, cyng::value_cast<std::string>(cfg["cluster"].get("pwd"), "")	//	ToDo: md5 + salt
			, cyng::value_cast(cfg["cluster"].get("monitor"), 60)
			, cyng::value_cast(cfg.get("session"), tmp));

		//
		//	server runtime configuration
		//
		const auto address = cyng::io::to_str(cfg["server"].get("address"));
		const auto service = cyng::io::to_str(cfg["server"].get("service"));

		CYNG_LOG_INFO(logger, "listener address: " << address);
		CYNG_LOG_INFO(logger, "listener service: " << service);
		srv.run(address, service);
		
		//
		//	wait for system signals
		//
		bool const shutdown = wait(logger);
		
		//
		//	close acceptor
		//
		CYNG_LOG_INFO(logger, "close acceptor");	
		srv.close();
		
		return shutdown;
	}		
}
