/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/cluster.h"
#include <NODE_project_info.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/set_cast.h>
#include <cyng/io/serializer.h>
//#include <cyng/json.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/rnd.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/random.hpp>

namespace node 
{
	//
	//	forward declaration(s):
	//
	void join_cluster(cyng::async::mux&
		, cyng::logging::log_ptr
		, boost::uuids::uuid
		, cyng::vector_t const& cfg_cluster
		, cyng::tuple_t cfg_db);

	controller::controller(unsigned int pool_size, std::string const& json_path, std::string node_name)
		: ctl(pool_size, json_path, node_name)
	{}

	cyng::vector_t controller::create_config(std::fstream& fout, boost::filesystem::path&& tmp, boost::filesystem::path&& cwd) const
	{
		//
		//	generate even distributed integers
		//
		cyng::crypto::rnd_num<int> rng(10, 120);

		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
			, cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", uidgen_())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
				, cyng::param_factory("load-config", "local")	//	options are local, master, mixed

				, cyng::param_factory("trigger", cyng::tuple_factory(
					cyng::param_factory("offset", 7),	//	minutes after midnight
					cyng::param_factory("frame", 3),	//	time frame in minutes
					cyng::param_factory("format", "SML")	//	supported formats are "SML" and "IEC"
				))

				, cyng::param_factory("profile-15min", cyng::tuple_factory(
					cyng::param_factory("root-dir", (cwd / "csv").string()),
					cyng::param_factory("prefix", "smf-15min-report-"),
                    cyng::param_factory("suffix", "csv"),
					cyng::param_factory("header", true),
					cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
				))

				, cyng::param_factory("profile-24h", cyng::tuple_factory(
					cyng::param_factory("root-dir", (cwd / "csv").string()),
					cyng::param_factory("prefix", "smf-24h-report-"),
                    cyng::param_factory("suffix", "csv"),
					cyng::param_factory("header", true),
					cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
				))

				, cyng::param_factory("DB", cyng::tuple_factory(
					cyng::param_factory("type", "SQLite"),
					cyng::param_factory("file-name", (cwd / "store.database").string()),
					cyng::param_factory("busy-timeout", 12),		//	seconds
					cyng::param_factory("watchdog", 30),	//	for database connection
					cyng::param_factory("pool-size", 1),	//	no pooling for SQLite
					cyng::param_factory("db-schema", NODE_SUFFIX),		//	use "v4.0" for compatibility to version 4.x
					cyng::param_factory("period", 12)	//	seconds
				))

				, cyng::param_factory("cluster", cyng::vector_factory({ cyng::tuple_factory(
					cyng::param_factory("host", "127.0.0.1"),
					cyng::param_factory("service", "7701"),
					cyng::param_factory("account", "root"),
					cyng::param_factory("pwd", NODE_PWD),
					cyng::param_factory("salt", NODE_SALT),
					cyng::param_factory("monitor", rng()),	//	seconds
					cyng::param_factory("group", 0)	//	customer ID
				) }))

			)
		});
	}

	bool controller::start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag)
	{
		//
		//	storage manager task
		//	Open database session only when needed (from task clock)
		//
		cyng::tuple_t tpl;

		//
		//	connect to cluster
		//
		cyng::vector_t vec;
		join_cluster(mux
			, logger
			, tag
			, cyng::value_cast(cfg.get("cluster"), vec)
			, cyng::value_cast(cfg.get("DB"), tpl));

		//
		//	wait for system signals
		//
		return wait(logger);
	}

	void join_cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, cyng::vector_t const& cfg_cluster
		, cyng::tuple_t cfg_db)
	{
		CYNG_LOG_TRACE(logger, "cluster redundancy: " << cfg_cluster.size());

		cyng::async::start_task_delayed<cluster>(mux
			, std::chrono::seconds(1)
			, logger
			, tag
			, load_cluster_cfg(cfg_cluster)
			, cyng::to_param_map(cfg_db));
	}
}
