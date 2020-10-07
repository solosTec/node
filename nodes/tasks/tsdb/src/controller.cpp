/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/cluster.h"
#include "tasks/single.h"
#include "tasks/multiple.h"
#include "tasks/line_protocol.h"
#include <NODE_project_info.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/set_cast.h>
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
	void join_cluster(cyng::async::mux&
		, cyng::logging::log_ptr
		, boost::uuids::uuid
		, bool force_mkdir
		, cyng::vector_t const& cfg_cluster
		, cyng::tuple_t cfg_db
		, cyng::tuple_t cfg_single
		, cyng::tuple_t cfg_multiple
		, cyng::tuple_t cfg_line_protocol
		, cyng::tuple_t cfg_influxdb
#if BOOST_OS_LINUX
		, cyng::tuple_t cfg_syslog
#endif
	);

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

		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
			, cyng::param_factory("log-level", "INFO")
			, cyng::param_factory("tag", get_random_tag())
			, cyng::param_factory("generated", std::chrono::system_clock::now())
			, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
			//, cyng::param_factory("load-config", "local")	//	options are local, master, mixed
			, cyng::param_factory("force-mkdir", false)	//	create output directories if not exists

			, cyng::param_factory("single-file", cyng::tuple_factory(
				cyng::param_factory("enabled", true),
				cyng::param_factory("root-dir", (cwd / "ts-report").string()),
				cyng::param_factory("prefix", "smf-full-report"),
                cyng::param_factory("suffix", "csv"),
				cyng::param_factory("period", 60),	//	seconds
				//cyng::param_factory("header", true),
				cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
			))

			, cyng::param_factory("multiple-files", cyng::tuple_factory(
				cyng::param_factory("enabled", false),
				cyng::param_factory("root-dir", (cwd / "ts-report").string()),
				cyng::param_factory("prefix", "smf-sub-report-"),
                cyng::param_factory("suffix", "csv"),
				cyng::param_factory("period", 60),	//	seconds
				cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
			))

			, cyng::param_factory("DB", cyng::tuple_factory(
				cyng::param_factory("enabled", false),
				cyng::param_factory("type", "SQLite"),
				cyng::param_factory("file-name", (cwd / "report.database").string()),
				cyng::param_factory("busy-timeout", 12),		//	seconds
				cyng::param_factory("watchdog", 30),	//	for database connection
				cyng::param_factory("pool-size", 1),	//	no pooling for SQLite
				cyng::param_factory("db-schema", NODE_SUFFIX),		//	use "v4.0" for compatibility to version 4.x
				cyng::param_factory("period", 12)	//	seconds
			))

			, cyng::param_factory("line-protocol", cyng::tuple_factory(
				cyng::param_factory("enabled", true),
				cyng::param_factory("root-dir", (cwd / "line-protocol").string()),
				cyng::param_factory("prefix", "smf-line-protocol"),
				cyng::param_factory("suffix", "txt"),
				cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
			))

			, cyng::param_factory("influxdb", cyng::tuple_factory(
				cyng::param_factory("enabled", false),
				cyng::param_factory("protocol", "JSON+UDP"),	//	HTTP, HTTPS, graphite
				cyng::param_factory("host", "http://localhost:8086/db/smf/series")	//	HTTP push
			))

#if BOOST_OS_LINUX
			, cyng::param_factory("syslog", cyng::tuple_factory(
				cyng::param_factory("enabled", false)
			))
#endif
			, cyng::param_factory("cluster", cyng::vector_factory({ cyng::tuple_factory(
				cyng::param_factory("host", "127.0.0.1"),
				cyng::param_factory("service", "7701"),
				cyng::param_factory("account", "root"),
				cyng::param_factory("pwd", NODE_PWD),
				cyng::param_factory("salt", NODE_SALT),
				cyng::param_factory("monitor", rng()),	//	seconds
				cyng::param_factory("group", 0)	//	customer ID
			) })))

			//
			//	ToDo: specify parameter for event logging (windows only)
			//	C:\WINDOWS\system32>EventCreate /t WARNING /id 118 /l APPLICATION /so "low session count" /d "below session count threshold"
			//	C:\WINDOWS\system32 > EventCreate /t WARNING /id 118 /l APPLICATION /so "SMF" /d "low session count"
			//
		});
	}

	bool controller::start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag)
	{
		auto const force_mkdir = cyng::value_cast(cfg.get("force-mkdir"), false);

		//
		//	connect to cluster
		//
		cyng::vector_t vec;
		cyng::tuple_t tpl;
		join_cluster(mux
			, logger
			, tag
			, force_mkdir
			, cyng::value_cast(cfg.get("cluster"), vec)
			, cyng::value_cast(cfg.get("DB"), tpl)
			, cyng::value_cast(cfg.get("single-file"), tpl)
			, cyng::value_cast(cfg.get("multiple-files"), tpl)
			, cyng::value_cast(cfg.get("line-protocol"), tpl)
			, cyng::value_cast(cfg.get("influxdb"), tpl)
#if BOOST_OS_LINUX
			, cyng::value_cast(cfg.get("syslog"), tpl)
#endif
		);

		//
		//	wait for system signals
		//
		return wait(logger);
	}


	void join_cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, bool force_mkdir
		, cyng::vector_t const& cfg_cluster
		, cyng::tuple_t cfg_db
		, cyng::tuple_t cfg_single
		, cyng::tuple_t cfg_multiple
		, cyng::tuple_t cfg_line_protocol
		, cyng::tuple_t cfg_influxdb
#if BOOST_OS_LINUX
		, cyng::tuple_t cfg_syslog
#endif
		)
	{
		CYNG_LOG_TRACE(logger, "cluster redundancy: " << cfg_cluster.size());
		auto const tmp = cyng::filesystem::temp_directory_path();

		//
		//	all storage tasks
		//
		std::set<std::size_t>	tasks;

		auto const dom_db = cyng::make_reader(cfg_db);
		if (cyng::value_cast(dom_db.get("enabled"), false)) {

			//
			//	start db task
			//
		}

		auto const dom_single = cyng::make_reader(cfg_single);
		if (cyng::value_cast(dom_single.get("enabled"), false)) {

			//
			//	test output directory
			//
			cyng::filesystem::path const dir = cyng::value_cast(dom_single.get("root-dir"), tmp.string());
			if (force_mkdir) {
				cyng::error_code ec;
				if (!cyng::filesystem::exists(dir, ec)) {
					CYNG_LOG_WARNING(logger, "create directory: " << dir);
					cyng::filesystem::create_directory(dir, ec);
				}
			}

			//
			//	start single file task
			//
			auto const r = cyng::async::start_task_sync<single>(mux, logger, dir
				, cyng::value_cast<std::string>(dom_single.get("prefix"), "smf-full-report")
				, cyng::value_cast<std::string>(dom_single.get("suffix"), "csv")
				, std::chrono::seconds(cyng::value_cast(dom_single.get("period"), 60)));
			if (r.second) {
				tasks.insert(r.first);
			}
			else {
				CYNG_LOG_FATAL(logger, "cannot start <single> task");
			}
		}

		auto const dom_multiple = cyng::make_reader(cfg_multiple);
		if (cyng::value_cast(dom_multiple.get("enabled"), false)) {

			//
			//	test output directory
			//
			cyng::filesystem::path const dir = cyng::value_cast(dom_single.get("root-dir"), tmp.string());
			if (force_mkdir) {
				cyng::error_code ec;
				if (!cyng::filesystem::exists(dir, ec)) {
					CYNG_LOG_WARNING(logger, "create directory: " << dir);
					cyng::filesystem::create_directory(dir, ec);
				}
			}

			//
			//	start multiple file task
			//
			auto const r = cyng::async::start_task_sync<multiple>(mux, logger, dir
				, cyng::value_cast<std::string>(dom_single.get("prefix"), "smf-sub-report-")
				, cyng::value_cast<std::string>(dom_single.get("suffix"), "csv")
				, std::chrono::seconds(cyng::value_cast(dom_single.get("period"), 60)));
			if (r.second) {
				tasks.insert(r.first);
			}
			else {
				CYNG_LOG_FATAL(logger, "cannot start <multiple> task");
			}
		}

		auto const dom_line_protocol = cyng::make_reader(cfg_line_protocol);
		if (cyng::value_cast(dom_line_protocol.get("enabled"), false)) {

			//
			//	test output directory
			//
			cyng::filesystem::path const dir = cyng::value_cast(dom_line_protocol.get("root-dir"), tmp.string());
			if (force_mkdir) {
				cyng::error_code ec;
				if (!cyng::filesystem::exists(dir, ec)) {
					CYNG_LOG_WARNING(logger, "create directory: " << dir);
					cyng::filesystem::create_directory(dir, ec);
				}
			}

			//
			//	start line protocol task
			//
			auto const r = cyng::async::start_task_sync<line_protocol>(mux, logger, dir
				, cyng::value_cast<std::string>(dom_line_protocol.get("prefix"), "smf-line-protocol")
				, cyng::value_cast<std::string>(dom_line_protocol.get("suffix"), "txt")
				, std::chrono::seconds(cyng::value_cast(dom_line_protocol.get("period"), 60)));
			if (r.second) {
				tasks.insert(r.first);
			}
			else {
				CYNG_LOG_FATAL(logger, "cannot start <line_protocol> task");
			}
		}

		auto const dom_influxdb = cyng::make_reader(cfg_influxdb);
		if (cyng::value_cast(dom_influxdb.get("enabled"), false)) {

			//
			//	start influxdb task
			//
		}

#if BOOST_OS_LINUX
		auto const dom_syslog = cyng::make_reader(cfg_syslog);
		if (cyng::value_cast(dom_syslog.get("enabled"), false)) {

			//
			//	start syslog task
			//
		}
#endif

		cyng::async::start_task_delayed<cluster>(mux
			, std::chrono::seconds(1)
			, logger
			, tag
			, load_cluster_cfg(cfg_cluster)
			, tasks);
	}
}
