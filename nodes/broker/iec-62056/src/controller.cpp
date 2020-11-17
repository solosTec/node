/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/cluster.h"
#include <NODE_project_info.h>

#include <cyng/async/task/task_builder.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/vector_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/rnd.h>

#include <boost/uuid/uuid_io.hpp>
#include <boost/random.hpp>

namespace node 
{
	//
	//	forward declaration(s):
	//
	std::size_t join_cluster(cyng::async::mux&
		, cyng::logging::log_ptr
		, boost::uuids::uuid
		, cyng::vector_t&&
		, cyng::tuple_t&&
		, cyng::tuple_t&&);

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
		//
		//	generate even distributed integers
		//
		cyng::crypto::rnd_num<int> rng(10, 120);

		return cyng::vector_factory(
			{ cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
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

			, cyng::param_factory("server", cyng::tuple_factory(
				cyng::param_factory("address", "0.0.0.0"),
				cyng::param_factory("service", "6000"))
			)
         
			, cyng::param_factory("client", cyng::tuple_factory(
				cyng::param_factory("login", false),
				cyng::param_factory("verbose", false))	//	parser	
			)

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
		//	connect to cluster
		//
		const auto tsk = join_cluster(mux
			, logger
			, tag
			, cyng::to_vector(cfg.get("cluster"))
			, cyng::to_tuple(cfg.get("server"))
			, cyng::to_tuple(cfg.get("client")));

		//
		//	wait for system signals
		//
		auto const shutdown = wait(logger);

		//
		//	stop cluster
		//
		CYNG_LOG_INFO(logger, "stop cluster");
		mux.stop(tsk);


		return shutdown;
	}

	std::size_t join_cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid cluster_tag
		, cyng::vector_t&& cfg_cls
		, cyng::tuple_t&& cfg_srv
		, cyng::tuple_t&& cfg_client)
	{
		CYNG_LOG_INFO(logger, "BUILD DATE: 2020-11-11");

		CYNG_LOG_TRACE(logger, "cluster redundancy: " << cfg_cls.size());

		auto const dom_srv = cyng::make_reader(cfg_srv);
		auto const dom_client = cyng::make_reader(cfg_client);

		auto const wd = cyng::filesystem::current_path();

		//	http::server build a string view
		try {
			auto address = cyng::value_cast(dom_srv.get("address"), "0.0.0.0");
			auto service = cyng::value_cast(dom_srv.get("service"), "2000");
			auto const host = cyng::make_address(address);
			const auto port = static_cast<unsigned short>(std::stoi(service));

			CYNG_LOG_INFO(logger, "address: " << address);
			CYNG_LOG_INFO(logger, "service: " << service);

			auto const client_login = cyng::value_cast(dom_client.get("login"), false);
			auto const verbose = cyng::value_cast(dom_client.get("verbose"), false);	// parser

			auto r = cyng::async::start_task_delayed<cluster>(mux
				, std::chrono::seconds(1)
				, logger
				, cluster_tag
				, load_cluster_cfg(cfg_cls)
				, boost::asio::ip::tcp::endpoint{ host, port }
				, client_login
				, verbose);

			if (r.second)	return r.first;
		}
		catch (std::exception const& ex) {
			CYNG_LOG_FATAL(logger, "could not start cluster: " << ex.what());
		}

		CYNG_LOG_FATAL(logger, "could not start cluster");
		return cyng::async::NO_TASK;
	}


}
