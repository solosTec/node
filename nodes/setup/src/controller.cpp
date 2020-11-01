/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/cluster.h"
#include "tasks/storage_db.h"
#include "tasks/storage_json.h"
#include "tasks/storage_xml.h"
#include <NODE_project_info.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/json.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/rnd.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/random.hpp>
#include <fstream>

namespace node 
{
	//
	//	forward declaration(s):
	//
	void join_cluster(cyng::async::mux&
		, cyng::logging::log_ptr
		, boost::uuids::uuid cluster_tag
		, cyng::store::db&
		, std::size_t
		, cyng::vector_t&&);

	std::size_t connect_data_store(cyng::async::mux&, cyng::logging::log_ptr, cyng::store::db&, std::string, cyng::tuple_t);

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
				
			, cyng::param_factory("input", "DB")	//	options are XML, JSON, DB

			, cyng::param_factory("DB", cyng::tuple_factory(
				cyng::param_factory("type", "SQLite"),
				cyng::param_factory("file-name", (cwd / "setup.database").string()),
				cyng::param_factory("busy-timeout", 12),		//	seconds
				cyng::param_factory("watchdog", 30),		//	for database connection
				cyng::param_factory("pool-size", 1)		//	no pooling for SQLite
			))
			, cyng::param_factory("XML", cyng::tuple_factory(
				cyng::param_factory("file-name", (cwd / "setup.config.xml").string()),
				cyng::param_factory("endcoding", "UTF-8")
			))
			, cyng::param_factory("JSON", cyng::tuple_factory(
				cyng::param_factory("file-name", (cwd / "setup.config.json").string())
			))
			, cyng::param_factory("cluster", cyng::vector_factory({ cyng::tuple_factory(
				cyng::param_factory("host", "127.0.0.1"),
				cyng::param_factory("service", "7701"),
				cyng::param_factory("account", "root"),
				cyng::param_factory("pwd", NODE_PWD),
				cyng::param_factory("salt", NODE_SALT),
				cyng::param_factory("monitor", rng())	//	seconds
			)})	))
		});
	}

	int controller::init_db(std::size_t idx, std::size_t conf_count)
	{
		//
		//	read configuration file
		//
		cyng::object config = cyng::json::read_file(json_path_);

		cyng::vector_t vec;
		vec = cyng::value_cast(config, vec);

		if (vec.size() > idx)
		{
			std::cout
				<< "***info: configuration index #"
				<< idx
				<< std::endl;

			auto dom = cyng::make_reader(vec[idx]);
			cyng::tuple_t tpl;
			return storage_db::init_db(cyng::value_cast(dom.get("DB"), tpl), conf_count);
		}
		else {
			std::cerr
				<< "***error: index of configuration vector is out of range "
				<< idx
				<< '/'
				<< vec.size()
				<< std::endl;
		}

		return EXIT_FAILURE;
	}

	int controller::generate_access_rights(std::size_t idx, std::size_t conf_count, std::string user)
	{
		//
		//	read configuration file
		//
		cyng::object config = cyng::json::read_file(json_path_);

		cyng::vector_t vec;
		vec = cyng::value_cast(config, vec);

		if (vec.size() > idx)
		{
			std::cout
				<< "***info: configuration index #"
				<< idx
				<< std::endl;

			auto dom = cyng::make_reader(vec[idx]);
			cyng::tuple_t tpl;
			return storage_db::generate_access_rights(cyng::value_cast(dom.get("DB"), tpl), conf_count, user);
		}
		else {
			std::cerr
				<< "***error: index of configuration vector is out of range "
				<< idx
				<< '/'
				<< vec.size()
				<< std::endl;
		}
		return EXIT_FAILURE;
	}

	bool controller::start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag)
	{
		//
		//	get configuration type
		//
		const auto config_type = cyng::io::to_str(cfg.get("input"));
		CYNG_LOG_TRACE(logger, "config type is " << config_type);

		/**
		 * global data cache
		 */
		cyng::store::db cache_;

		//
		//	storage manager task
		//
		cyng::tuple_t tpl;
		std::size_t storage_task = connect_data_store(mux, logger, cache_, config_type, cyng::value_cast(cfg.get(config_type), tpl));

		//
		//	connect to cluster
		//
		join_cluster(mux
			, logger
			, tag
			, cache_
			, storage_task
			, cyng::to_vector(cfg.get("cluster")));

		//
		//	wait for system signals
		//
		return wait(logger);

	}

	void join_cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid cluster_tag
		, cyng::store::db& cache
		, std::size_t tsk
		, cyng::vector_t&& cfg)
	{
		CYNG_LOG_TRACE(logger, "cluster redundancy: " << cfg.size());

		cyng::async::start_task_delayed<cluster>(mux
			, std::chrono::seconds(1)
			, logger
			, cluster_tag
			, cache
			, tsk
			, load_cluster_cfg(cfg));

	}

	std::size_t connect_data_store(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cyng::store::db& cache
		, std::string config_type
		, cyng::tuple_t cfg)
	{
		if (boost::algorithm::iequals(config_type, "DB"))
		{
			CYNG_LOG_INFO(logger, "connect to configuration database");

			return cyng::async::start_task_delayed<storage_db>(mux
				, std::chrono::seconds(1)
				, logger
				, cache
				, cyng::to_param_map(cfg)).first;
		}
		else if (boost::algorithm::iequals(config_type, "XML"))
		{
			CYNG_LOG_INFO(logger, "open XML configuration files");

			return cyng::async::start_task_delayed<storage_xml>(mux
				, std::chrono::seconds(1)
				, logger
				, cache
				, cyng::to_param_map(cfg)).first;
		}
		else if (boost::algorithm::iequals(config_type, "JSON"))
		{
			CYNG_LOG_INFO(logger, "open JSONL configuration files");

			return cyng::async::start_task_delayed<storage_json>(mux
				, std::chrono::seconds(1)
				, logger
				, cache
				, cyng::to_param_map(cfg)).first;
		}
		CYNG_LOG_FATAL(logger, "unknown config type " << config_type);
		return cyng::async::NO_TASK;
	}

}
