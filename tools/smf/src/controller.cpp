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
#include <cyng/set_cast.h>
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

	cyng::vector_t controller::create_config(std::fstream& fout, cyng::filesystem::path&& tmp, cyng::filesystem::path&& cwd) const
	{	
		cyng::crypto::rnd_num<int> rng(10, 60);

		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", get_random_tag())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
				, cyng::param_factory("intercative", true)
				, cyng::param_factory("country-code", "CH")
				, cyng::param_factory("language-code", "EN")

				, cyng::param_factory("tracking", cyng::tuple_factory(
					cyng::param_factory("path", (cwd / "project-tracking.csv").string()),
					cyng::param_factory("auto-save", 120)	//	seconds
				))

				, cyng::param_factory("cluster", cyng::vector_factory({ cyng::tuple_factory(
					cyng::param_factory("host", "127.0.0.1"),
					cyng::param_factory("service", "7701"),
					cyng::param_factory("account", "root"),
					cyng::param_factory("pwd", NODE_PWD),
					cyng::param_factory("salt", NODE_SALT),
					cyng::param_factory("monitor", rng())	//	seconds
				)}))

			)
		});
	}
	
	bool controller::start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag)
	{
		auto const intercative = cyng::value_cast(cfg.get("intercative"), true);
		auto const country_code = cyng::value_cast<std::string>(cfg.get("country-code"), "CH");
		auto const language_code = cyng::value_cast<std::string>(cfg.get("language-code"), "EN");

		auto const vec = cyng::to_vector(cfg.get("cluster"));

		cli term(mux, logger, tag, load_cluster_cfg(vec), std::cout, std::cin);
		term.run();
		
		bool const shutdown{ true };
		return shutdown;
	}		
}
