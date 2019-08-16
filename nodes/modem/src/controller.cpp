/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/cluster.h"
#include <NODE_project_info.h>
#include <cyng/factory/set_factory.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/rnd.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/random.hpp>
#include <iostream>
#include <chrono>

namespace node 
{
	//
	//	forward declaration(s):
	//
	void join_cluster(cyng::async::mux&
		, cyng::logging::log_ptr
		, boost::uuids::uuid cluster_tag
		, cyng::vector_t const&
		, cyng::tuple_t const&);

	controller::controller(unsigned int pool_size, std::string const& json_path, std::string node_name)
		: ctl(pool_size, json_path, node_name)
	{}

	cyng::vector_t controller::create_config(std::fstream& fout, boost::filesystem::path&& tmp, boost::filesystem::path&& cwd) const
	{
		//
		//	generate even distributed integers
		//	reconnect to master on different times
		//
		cyng::crypto::rnd_num<int> rng(10, 120);

		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", uidgen_())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))

				, cyng::param_factory("server", cyng::tuple_factory(
					cyng::param_factory("address", "0.0.0.0"),
					cyng::param_factory("service", "9000"),
					cyng::param_factory("auto-answer", true),	//	accept incoming calls automatically
					cyng::param_factory("guard-time", 1000),	//	milliseconds
					cyng::param_factory("timeout", 12)		//	connection timeout
				))
				, cyng::param_factory("cluster", cyng::vector_factory({ cyng::tuple_factory(
					cyng::param_factory("host", "127.0.0.1"),
					cyng::param_factory("service", "7701"),
					cyng::param_factory("account", "root"),
					cyng::param_factory("pwd", NODE_PWD),
					cyng::param_factory("salt", NODE_SALT),
					cyng::param_factory("monitor", rng()),	//	seconds
					cyng::param_factory("auto-config", false),	//	client security
					cyng::param_factory("group", 0)	//	customer ID
				) }))
			)
		});
	}

	bool controller::start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag)
	{
		//
		//	connect to cluster
		//
		cyng::vector_t tmp_vec;
		cyng::tuple_t tmp_tpl;
		join_cluster(mux
			, logger
			, tag
			, cyng::value_cast(cfg.get("cluster"), tmp_vec)
			, cyng::value_cast(cfg.get("server"), tmp_tpl));

		//
		//	wait for system signals
		//
		return wait(logger);
	}

	void join_cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid cluster_tag
		, cyng::vector_t const& cfg_cluster
		, cyng::tuple_t const& cfg_srv)
	{
		CYNG_LOG_TRACE(logger, "cluster redundancy: " << cfg_cluster.size());

		auto dom = cyng::make_reader(cfg_srv);

		std::chrono::milliseconds guard_time = std::chrono::milliseconds(cyng::value_cast<int>(dom.get("guard-time"), 1000));
		cyng::async::start_task_delayed<cluster>(mux
			, std::chrono::seconds(1)
			, logger
			, cluster_tag
			, load_cluster_cfg(cfg_cluster)
			, cyng::value_cast<std::string>(dom.get("address"), "0.0.0.0")
			, cyng::value_cast<std::string>(dom.get("service"), "9000")
			, cyng::value_cast<int>(dom.get("timeout"), 12)
			, cyng::value_cast(dom.get("auto-answer"), true)
			, guard_time);

	}

}
