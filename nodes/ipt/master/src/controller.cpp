/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/cluster.h"
#include <NODE_project_info.h>
#include <smf/ipt/scramble_key_io.hpp>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
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
	std::pair<std::size_t, bool> join_cluster(cyng::async::mux&
		, cyng::logging::log_ptr
		, boost::uuids::uuid cluster_tag
		, cyng::vector_t const&
		, cyng::tuple_t const&
		, bool sml_log);

	controller::controller(unsigned int pool_size, std::string const& json_path, std::string node_name)
		: ctl(pool_size, json_path, node_name)
	{}
	
	cyng::vector_t controller::create_config(std::fstream& fout, boost::filesystem::path&& tmp, boost::filesystem::path&& cwd) const
	{

		//
		//	reconnect to master on different times
		//
		cyng::crypto::rnd_num<int> rnd_monitor(10, 60);

		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
			, cyng::param_factory("log-level", "INFO")
			, cyng::param_factory("tag", uidgen_())
			, cyng::param_factory("generated", std::chrono::system_clock::now())
			, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))

			, cyng::param_factory("server", cyng::tuple_factory(
				cyng::param_factory("address", "0.0.0.0"),
				cyng::param_factory("service", "26862"),
				cyng::param_factory("sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
				cyng::param_factory("watchdog", 30),	//	for IP-T connection (minutes)
				cyng::param_factory("timeout", 10)		//	connection timeout in seconds
			))
			, cyng::param_factory("sml-log", false)		//	log SML parser
			, cyng::param_factory("cluster", cyng::vector_factory({ cyng::tuple_factory(
				cyng::param_factory("host", "127.0.0.1"),
				cyng::param_factory("service", "7701"),
				cyng::param_factory("account", "root"),
				cyng::param_factory("pwd", NODE_PWD),
				cyng::param_factory("salt", NODE_SALT),
				cyng::param_factory("monitor", rnd_monitor()),	//	seconds
				cyng::param_factory("auto-config", false),	//	client security
				cyng::param_factory("group", 0)	//	customer ID
			) })))
		});

	}

	bool controller::start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag)
	{
		//
		//	SML parser logging
		//
		auto const log_sml = cyng::value_cast(cfg.get("sml-log"), false);
		if (log_sml) {
			CYNG_LOG_INFO(logger, "SML logging is ON");
		}
		else {
			CYNG_LOG_INFO(logger, "SML logging is OFF");
		}

		//
		//	connect to cluster
		//
		cyng::vector_t tmp_vec;
		cyng::tuple_t tmp_tpl;
		std::pair<std::size_t, bool> const r = join_cluster(mux
			, logger
			, tag
			, cyng::value_cast(cfg.get("cluster"), tmp_vec)
			, cyng::value_cast(cfg.get("server"), tmp_tpl)
			, log_sml);

		bool shutdown{ true };
		if (r.second) {

			//
			//	wait for system signals
			//
			shutdown = wait(logger);

			//
			//	cluster tasks
			//
			CYNG_LOG_INFO(logger, "stop cluster task #" << r.first);
			mux.stop(r.first);
		}
		else {

			CYNG_LOG_FATAL(logger, "couldn't start cluster task");
		}


		return shutdown;
	}

	std::pair<std::size_t, bool> join_cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid cluster_tag
		, cyng::vector_t const& cfg_cls
		, cyng::tuple_t const& cfg_srv
		, bool sml_log)
	{
		CYNG_LOG_TRACE(logger, "cluster redundancy: " << cfg_cls.size());

		auto dom = cyng::make_reader(cfg_srv);

		//
		//	read default scramble key
		//
		std::stringstream ss;
		ipt::scramble_key sk;
		ss << cyng::value_cast<std::string>(dom.get("sk"), "0102030405060708090001020304050607080900010203040506070809000001");
		ss >> sk;

		return cyng::async::start_task_delayed<cluster>(mux
			, std::chrono::seconds(1)
			, logger
			, cluster_tag
			, load_cluster_cfg(cfg_cls)
			, cyng::value_cast<std::string>(dom.get("address"), "0.0.0.0")
			, cyng::value_cast<std::string>(dom.get("service"), "26862")
			, sk
			, cyng::value_cast<int>(dom.get("watchdog"), 30)
			, cyng::value_cast<int>(dom.get("timeout"), 12)
			, sml_log);

	}

}
