/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/network.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/json.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/vector_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <cyng/compatibility/file_system.hpp>
#include <fstream>

namespace node 
{
	//
	//	forward declaration(s):
	//
	void join_network(cyng::async::mux&, cyng::logging::log_ptr, cyng::vector_t const&);

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
			, cyng::param_factory("log-level", "INFO")
			, cyng::param_factory("tag", get_random_tag())
			, cyng::param_factory("generated", std::chrono::system_clock::now())
			, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))

			, cyng::param_factory("ipt", cyng::vector_factory({
				cyng::tuple_factory(
					cyng::param_factory("host", "127.0.0.1"),
					cyng::param_factory("service", "26862"),
					cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
					cyng::param_factory("scrambled", true),
					cyng::param_factory("monitor", 57)),	//	seconds
				cyng::tuple_factory(
					cyng::param_factory("host", "127.0.0.1"),
					cyng::param_factory("service", "26863"),
					cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
					cyng::param_factory("scrambled", false),
					cyng::param_factory("monitor", 57))
				}))
			, cyng::param_factory("collector", cyng::vector_factory({
				cyng::tuple_factory(
					cyng::param_factory("address", "0.0.0.0"),
					cyng::param_factory("service", "33001"),
 					cyng::param_factory("account", "collector-33001"),
 					cyng::param_factory("pwd", "to-define"),
					cyng::param_factory("connection-open-retries", 1),	//	be careful! value one is highly recommended
					cyng::param_factory("tag", get_random_tag())),
				cyng::tuple_factory(
					cyng::param_factory("address", "0.0.0.0"),
					cyng::param_factory("service", "33002"),
 					cyng::param_factory("account", "collector-33002"),
 					cyng::param_factory("pwd", "to-define"),
					cyng::param_factory("connection-open-retries", 1),	//	be careful! value one is highly recommended
					cyng::param_factory("tag", get_random_tag()))
				}))
			)
		});
	}

	bool controller::start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag)
	{
		//
		//	connect to cluster
		//
		cyng::vector_t tmp;
		join_network(mux
			, logger
			, cyng::value_cast(cfg.get("ipt"), tmp));

		//
		//	wait for system signals
		//
		const bool shutdown = wait(logger);

		//
		//	close acceptor
		//
		CYNG_LOG_INFO(logger, "close acceptor");
		//srv.close();


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
