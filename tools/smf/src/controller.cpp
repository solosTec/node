/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "cli.h"
#include <NODE_project_info.h>

#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/value_cast.hpp>
#include <cyng/async/mux.h>

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

	cyng::vector_t controller::create_config(std::fstream& fout, boost::filesystem::path&& tmp, boost::filesystem::path&& cwd) const
	{	
		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", uidgen_())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
				, cyng::param_factory("intercative", true)
				, cyng::param_factory("country-code", "CH")
				, cyng::param_factory("language-code", "EN")

				, cyng::param_factory("tracking", cyng::tuple_factory(
					cyng::param_factory("path", (cwd / "project-tracking.csv").string()),
					cyng::param_factory("auto-save", 120)	//	seconds
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
		auto const intercative = cyng::value_cast(cfg.get("intercative"), true);
		auto const country_code = cyng::value_cast<std::string>(cfg.get("country-code"), "CH");
		auto const language_code = cyng::value_cast<std::string>(cfg.get("language-code"), "EN");

		cli term(mux.get_io_service(), tag, std::cout, std::cin);
		term.run();
		
		bool const shutdown{ true };
		//while (!shutdown) {

		//}
		return shutdown;
	}		
}
