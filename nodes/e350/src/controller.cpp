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
#include <cyng/json.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/rnd.h>
#include <cyng/vector_cast.hpp>
#include <cyng/set_cast.h>

#include <boost/uuid/uuid_io.hpp>
#include <boost/random.hpp>

namespace node 
{
	//
	//	forward declaration(s):
	//
	void join_cluster(cyng::async::mux&
		, cyng::logging::log_ptr
		, boost::uuids::uuid cluster_tag
		, cyng::vector_t&&
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
		//	generate a random number
		//
		cyng::crypto::rnd_num<int> rnd_monitor(10, 60);

		//
		//	generate a random string
		//
		auto rnd_str = cyng::crypto::make_rnd_alnum();

		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
                , cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", uidgen_())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))

				, cyng::param_factory("server", cyng::tuple_factory(
					cyng::param_factory("address", "0.0.0.0"),
					cyng::param_factory("service", "5200"),
					cyng::param_factory("timeout", 12),		//	connection timeout
					cyng::param_factory("watchdog", 30),	//	minutes
					cyng::param_factory("pwd-policy", "global"),	// swibi/MNAME, sgsw/TELNB
					cyng::param_factory("global-pwd", rnd_str.next(8)),	//	8 characters
					cyng::param_factory("blocklist", cyng::vector_factory({
						cyng::make_address("185.244.25.187"),	//	KV Solutions B.V. scans for "login.cgi"
						cyng::make_address("139.219.100.104"),	//	ISP Microsoft (China) Co. Ltd. - 2018-07-31T21:14
						cyng::make_address("194.147.32.109"),	//	Udovikhin Evgenii - 2019-02-01 15:23:08.13699453
						cyng::make_address("185.209.0.12"),		//	2019-03-27 11:23:39
						cyng::make_address("42.236.101.234"),	//	hn.kd.ny.adsl (china)
						cyng::make_address("185.104.184.126"),	//	M247 Ltd
						cyng::make_address("185.162.235.56")	//	SILEX malware
					}))	//	blocklist
				))
				, cyng::param_factory("cluster", cyng::vector_factory({ cyng::tuple_factory(
					cyng::param_factory("host", "127.0.0.1"),
					cyng::param_factory("service", "7701"),
					cyng::param_factory("account", "root"),
					cyng::param_factory("pwd", NODE_PWD),
					cyng::param_factory("salt", NODE_SALT),
					cyng::param_factory("monitor", rnd_monitor.next()),	//	seconds
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
		join_cluster(mux
			, logger
			, tag
			, cyng::to_vector(cfg.get("cluster"))
			, cyng::to_tuple(cfg.get("server")));

		//
		//	wait for system signals
		//
		return wait(logger);
	}

	void join_cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid cluster_tag
		, cyng::vector_t&& cfg_cluster
		, cyng::tuple_t&& cfg_srv)
	{
		CYNG_LOG_TRACE(logger, "cluster redundancy: " << cfg_cluster.size());

		auto dom = cyng::make_reader(cfg_srv);

		//
		//	get blocklisted addresses
		//
		const auto blocklist_str = cyng::vector_cast<std::string>(dom.get("blocklist"), "");
		CYNG_LOG_INFO(logger, blocklist_str.size() << " addresses are blocklisted");
		std::set<boost::asio::ip::address>	blocklist;
		for (auto const& a : blocklist_str) {
			auto r = blocklist.insert(boost::asio::ip::make_address(a));
			if (r.second) {
				CYNG_LOG_TRACE(logger, *r.first);
			}
			else {
				CYNG_LOG_WARNING(logger, "cannot insert " << a);
			}
		}

		//
		//	generate a random string
		//
		auto rnd_str = cyng::crypto::make_rnd_alnum();

		cyng::async::start_task_delayed<cluster>(mux
			, std::chrono::seconds(1)
			, logger
			, cluster_tag
			, load_cluster_cfg(cfg_cluster)
			, cyng::value_cast<std::string>(dom.get("address"), "0.0.0.0")
			, cyng::value_cast<std::string>(dom.get("service"), "6000")
			, std::chrono::seconds(cyng::value_cast<int>(dom.get("timeout"), 12))
			, blocklist
			, cyng::value_cast<std::string>(dom.get("pwd-policy"), "global")
			, cyng::value_cast(dom.get("global-pwd"), rnd_str(8))
			);

	}

}
