/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include <NODE_project_info.h>
#include "tasks/sml_to_db_consumer.h"
#include "tasks/sml_to_xml_consumer.h"
#include "tasks/sml_to_abl_consumer.h"
#include "tasks/binary_consumer.h"
#include "tasks/sml_to_json_consumer.h"
#include "tasks/sml_to_log_consumer.h"
#include "tasks/sml_to_csv_consumer.h"
#include "tasks/iec_to_db_consumer.h"
#include "tasks/network.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>
#include <cyng/json.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/vector_cast.hpp>
#include <cyng/parser/version_parser.h>
#include <cyng/rnd.h>

#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>

namespace node 
{
	//
	//	forward declaration(s):
	//

	/**
	 * start ipt client
	 */
	std::size_t join_network(cyng::async::mux&
		, cyng::logging::log_ptr
		, boost::uuids::uuid tag
		, bool log_pushdata
		, cyng::vector_t const&
		, cyng::param_map_t);

	/**
	 * start all data consumer tasks
	 */
	std::vector<std::size_t> connect_data_store(cyng::async::mux&
		, cyng::logging::log_ptr
		, std::vector<std::string> const&
		, std::size_t ntid	//	network task id
		, cyng::reader<cyng::object> const& cfg);

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
		cyng::crypto::rnd_num<int> rng(10, 60);

		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
			, cyng::param_factory("log-level", "INFO")
			, cyng::param_factory("tag", uidgen_())
			, cyng::param_factory("generated", std::chrono::system_clock::now())
			, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
				
			, cyng::param_factory("log-pushdata", false)	//	log file for each channel
			, cyng::param_factory("output", cyng::vector_factory({"ALL:BIN"}))	//	options are XML, JSON, DB, BIN, ...

			, cyng::param_factory("SML:DB", cyng::tuple_factory(
				cyng::param_factory("type", "SQLite"),
				cyng::param_factory("file-name", (cwd / "store.database").string()),
				cyng::param_factory("busy-timeout", 12),		//	seconds
				cyng::param_factory("watchdog", 30),	//	for database connection
				cyng::param_factory("pool-size", 1),	//	no pooling for SQLite
				cyng::param_factory("db-schema", NODE_SUFFIX),		//	use "v4.0" for compatibility to version 4.x
				cyng::param_factory("period", rng())	//	seconds
			))
			, cyng::param_factory("IEC:DB", cyng::tuple_factory(
				cyng::param_factory("type", "SQLite"),
				cyng::param_factory("file-name", (cwd / "store.database").string()),
				cyng::param_factory("busy-timeout", 12),		//	seconds
				cyng::param_factory("watchdog", 30),	//	for database connection
				cyng::param_factory("pool-size", 1),	//	no pooling for SQLite
				cyng::param_factory("db-schema", NODE_SUFFIX),
				cyng::param_factory("period", rng()),	//	seconds
				cyng::param_factory("ignore-null", false)	//	don't write values equal 0
			))
			, cyng::param_factory("SML:XML", cyng::tuple_factory(
				cyng::param_factory("root-dir", (cwd / "xml").string()),
				cyng::param_factory("root-name", "SML"),
				cyng::param_factory("endcoding", "UTF-8"),
				cyng::param_factory("period", rng())	//	seconds
			))
			, cyng::param_factory("SML:JSON", cyng::tuple_factory(
				cyng::param_factory("root-dir", (cwd / "json").string()),
				cyng::param_factory("prefix", "smf"),
				cyng::param_factory("suffix", "json"),
				cyng::param_factory("version", NODE_SUFFIX),
				cyng::param_factory("period", rng())	//	seconds
			))
			, cyng::param_factory("SML:ABL", cyng::tuple_factory(
				cyng::param_factory("root-dir", (cwd / "abl").string()),
				cyng::param_factory("prefix", "smf"),
				cyng::param_factory("suffix", "abl"),
				cyng::param_factory("version", NODE_SUFFIX),
				cyng::param_factory("period", rng()),	//	seconds
				cyng::param_factory("line-ending", "DOS")	//	DOS/UNIX
			))
			, cyng::param_factory("ALL:BIN", cyng::tuple_factory(
				cyng::param_factory("root-dir", (cwd / "sml").string()),
				cyng::param_factory("prefix", "smf"),
				cyng::param_factory("suffix", "sml"),
				cyng::param_factory("version", NODE_SUFFIX),
				cyng::param_factory("period", rng())
			))
			, cyng::param_factory("SML:LOG", cyng::tuple_factory(
				cyng::param_factory("root-dir", (cwd / "log").string()),
				cyng::param_factory("prefix", "sml"),
				cyng::param_factory("suffix", "log"),
				cyng::param_factory("version", NODE_SUFFIX),
				cyng::param_factory("period", rng())	//	seconds
			))
			, cyng::param_factory("IEC:LOG", cyng::tuple_factory(
				cyng::param_factory("root-dir", (cwd / "log").string()),
				cyng::param_factory("prefix", "iec"),
				cyng::param_factory("suffix", "log"),
				cyng::param_factory("version", NODE_SUFFIX),
				cyng::param_factory("period", rng())	//	seconds
			))
			, cyng::param_factory("SML:CSV", cyng::tuple_factory(
				cyng::param_factory("root-dir", (cwd / "csv").string()),
				cyng::param_factory("prefix", "smf"),
				cyng::param_factory("suffix", "csv"),
				cyng::param_factory("header", true),
				cyng::param_factory("version", NODE_SUFFIX),
				cyng::param_factory("period", rng())	//	seconds
			))
			, cyng::param_factory("ipt", cyng::vector_factory({
				cyng::tuple_factory(
					cyng::param_factory("host", "127.0.0.1"),
					cyng::param_factory("service", "26862"),
					cyng::param_factory("account", "data-store"),
					cyng::param_factory("pwd", "to-define"),
					cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
					cyng::param_factory("scrambled", true),
					cyng::param_factory("monitor", rng())),	//	seconds
				cyng::tuple_factory(
					cyng::param_factory("host", "127.0.0.1"),
					cyng::param_factory("service", "26863"),
					cyng::param_factory("account", "data-store"),
					cyng::param_factory("pwd", "to-define"),
					cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
					cyng::param_factory("scrambled", false),
					cyng::param_factory("monitor", rng()))
				}))
				, cyng::param_factory("targets", cyng::vector_factory({ 
					cyng::param_factory("data.sink.sml", "SML"),
					cyng::param_factory("data.sink.iec", "IEC") }))	//	list of targets and their data type
				)
			});
	}

	int controller::init_db(std::size_t idx)
	{
		//
		//	read configuration file
		//
		cyng::object config = cyng::json::read_file(json_path_);

		cyng::vector_t vec;
		vec = cyng::value_cast(config, vec);
		BOOST_ASSERT_MSG(!vec.empty(), "invalid configuration");

		if (vec.size() > idx)
		{
			std::cerr
				<< "***info: configuration index #"
				<< idx
				<< '/'
				<< vec.size()
				<< std::endl;

			auto dom = cyng::make_reader(vec[idx]);
			cyng::tuple_t tpl;
			return (EXIT_SUCCESS == sml_db_consumer::init_db(cyng::value_cast(dom.get("SML:DB"), tpl)))
				&& (EXIT_SUCCESS == iec_db_consumer::init_db(cyng::value_cast(dom.get("IEC:DB"), tpl)));
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
        //  control logging of push data
        //
		const auto log_pushdata = cyng::value_cast(cfg.get("log-pushdata"), false);
		
		//
		//	connect to ipt master
		//
		cyng::vector_t tmp;
		auto ntid = join_network(mux
			, logger
			, tag
			, log_pushdata
			, cyng::value_cast(cfg.get("ipt"), tmp)
			, cyng::to_param_map(cyng::value_cast(cfg.get("targets"), tmp)));

		//
		//	get configuration type
		//
		const auto config_types = cyng::vector_cast<std::string>(cfg.get("output"), "");

		//
		//	start all consumer tasks
		//
		cyng::tuple_t tpl;
		auto tsks = connect_data_store(mux, logger, config_types, ntid, cfg);
		if (tsks.empty())
		{
			CYNG_LOG_FATAL(logger, "no output channels found");
			return true;	//	shutdown
		}
		CYNG_LOG_TRACE(logger, tsks.size() << " output channel(s) defined");

		//
		//	wait for system signals
		//
		return wait(logger);
	}

	/**
	 * start all data consumer tasks
	 */
	std::vector<std::size_t> connect_data_store(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, std::vector<std::string> const& config_types
		, std::size_t ntid	//	network task id
		, cyng::reader<cyng::object> const& cfg)
	{
		std::vector<std::size_t> tsks;
		cyng::tuple_t tpl;
		const boost::filesystem::path pwd = boost::filesystem::current_path();

		for (const auto& config_type : config_types)
		{
			if (boost::algorithm::iequals(config_type, "SML:DB"))
			{
				CYNG_LOG_INFO(logger, "start database adapter for SML protocol");

				tpl = cyng::value_cast(cfg.get(config_type), tpl);

				tsks.push_back(cyng::async::start_task_delayed<sml_db_consumer>(mux
					, std::chrono::seconds(1)
					, logger
					, ntid
					, cyng::to_param_map(tpl)).first);
			}
			else if (boost::algorithm::iequals(config_type, "IEC:DB"))
			{
				CYNG_LOG_INFO(logger, "start database adapter for IEC 62056-21 protocol (readout mode)");

				tpl = cyng::value_cast(cfg.get(config_type), tpl);

				tsks.push_back(cyng::async::start_task_delayed<iec_db_consumer>(mux
					, std::chrono::seconds(1)
					, logger
					, ntid
					, cyng::to_param_map(tpl)).first);
			}
			else if (boost::algorithm::iequals(config_type, "SML:XML"))
			{
				CYNG_LOG_INFO(logger, "start SML:XML consumer");

				tpl = cyng::value_cast(cfg.get(config_type), tpl);

				boost::filesystem::path root_dir = cyng::value_cast(cfg[config_type].get("root-dir"), (pwd / "xml").string());
				auto root_name = cyng::value_cast<std::string>(cfg[config_type].get("root-name"), "SML");
				auto encoding = cyng::value_cast<std::string>(cfg[config_type].get("endcoding"), "UTF-8");
				auto period = cyng::value_cast(cfg[config_type].get("period"), 16);	//	seconds

				tsks.push_back(cyng::async::start_task_delayed<sml_xml_consumer>(mux
					, std::chrono::seconds(1)
					, logger
					, ntid
					, root_dir
					, root_name
					, encoding
					, std::chrono::seconds(period)).first);
			}
			else if (boost::algorithm::iequals(config_type, "SML:JSON"))
			{
				CYNG_LOG_INFO(logger, "start SML:JSON consumer");

				tpl = cyng::value_cast(cfg.get(config_type), tpl);

				boost::filesystem::path root_dir = cyng::value_cast(cfg[config_type].get("root-dir"), (pwd / "json").string());
				auto prefix = cyng::value_cast<std::string>(cfg[config_type].get("prefix"), "sml");
				auto suffix = cyng::value_cast<std::string>(cfg[config_type].get("suffix"), "json");

				tsks.push_back(cyng::async::start_task_delayed<sml_json_consumer>(mux
					, std::chrono::seconds(1)
					, logger
					, ntid
					, root_dir
					, prefix
					, suffix).first);
			}
			else if (boost::algorithm::iequals(config_type, "SML:ABL"))
			{
				CYNG_LOG_INFO(logger, "start SML:ABL consumer");

				tpl = cyng::value_cast(cfg.get(config_type), tpl);

				boost::filesystem::path root_dir = cyng::value_cast(cfg[config_type].get("root-dir"), (pwd / "abl").string());
				auto prefix = cyng::value_cast<std::string>(cfg[config_type].get("prefix"), "sml");
				auto suffix = cyng::value_cast<std::string>(cfg[config_type].get("suffix"), "abl");
				auto version = cyng::value_cast<std::string>(cfg[config_type].get("version"), NODE_SUFFIX);
				auto r = cyng::parse_ver(version);
				auto period = cyng::value_cast(cfg[config_type].get("period"), 16);	//	seconds
				auto eol = cyng::value_cast<std::string>(cfg[config_type].get("line-ending"), "DOS");
				

				tsks.push_back(cyng::async::start_task_delayed<sml_abl_consumer>(mux
					, std::chrono::seconds(1)
					, logger
					, ntid
					, root_dir
					, prefix
					, suffix
					, std::chrono::seconds(period)
					, boost::algorithm::equals(eol, "DOS")
					, r.second ? r.first : cyng::make_object<cyng::version>(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)).first);
			}
			else if (boost::algorithm::iequals(config_type, "ALL:BIN"))
			{
				CYNG_LOG_INFO(logger, "start ALL:BIN consumer");

				auto root_dir = cyng::value_cast(cfg[config_type].get("root-dir"), (pwd / "sml").string());
				auto prefix = cyng::value_cast<std::string>(cfg[config_type].get("prefix"), "sml");
				auto suffix = cyng::value_cast<std::string>(cfg[config_type].get("suffix"), "sml");
				auto period = cyng::value_cast(cfg[config_type].get("period"), 16);	//	seconds

				tsks.push_back(cyng::async::start_task_delayed<binary_consumer>(mux
					, std::chrono::seconds(1)
					, logger
					, ntid
					, root_dir
					, prefix
					, suffix
					, std::chrono::seconds(period)).first);
			}
			else if (boost::algorithm::iequals(config_type, "SML:LOG"))
			{
				CYNG_LOG_INFO(logger, "start SML:LOG consumer");

				tpl = cyng::value_cast(cfg.get(config_type), tpl);

				tsks.push_back(cyng::async::start_task_delayed<sml_log_consumer>(mux
					, std::chrono::seconds(1)
					, logger
					, ntid
					, cyng::to_param_map(tpl)).first);
			}
			else if (boost::algorithm::iequals(config_type, "SML:CSV"))
			{
				CYNG_LOG_INFO(logger, "start SML:CSV storage");

				//auto version = cyng::value_cast(dom[config_type].get("version"), cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR).to_double());
				boost::filesystem::path root_dir = cyng::value_cast(cfg[config_type].get("root-dir"), (pwd / "csv").string());
				auto prefix = cyng::value_cast<std::string>(cfg[config_type].get("prefix"), "smf");
				auto suffix = cyng::value_cast<std::string>(cfg[config_type].get("suffix"), "csv");
				auto header = cyng::value_cast(cfg[config_type].get("header"), true);
				auto period = cyng::value_cast(cfg[config_type].get("period"), 16);	//	seconds

				tsks.push_back(cyng::async::start_task_delayed<sml_csv_consumer>(mux
					, std::chrono::seconds(1)
					, logger
					, ntid
					, root_dir
					, prefix
					, suffix
					, header
					, std::chrono::seconds(period)).first);
			}
			else
			{
				CYNG_LOG_ERROR(logger, "unknown config type " << config_type);
			}

			tpl.clear();
		}
		return tsks;
	}

	/**
	 * start ipt client
	 */
	std::size_t join_network(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, bool log_pushdata
		, cyng::vector_t const& cfg
		, cyng::param_map_t targets)
	{
		CYNG_LOG_TRACE(logger, "ipt network redundancy: " << cfg.size());

		/**
		 * log all configured targets and their protocol types
		 */
		std::map<std::string, std::string> target_list;
		for (auto const& target : targets)
		{
			auto pos = target_list.emplace(target.first, cyng::value_cast<std::string>(target.second, ""));
			CYNG_LOG_TRACE(logger, "target " << pos.first->first << ':' << pos.first->second);
		}

		BOOST_ASSERT(!cfg.empty());
		return cyng::async::start_task_detached<ipt::network>(mux
			, logger
			, tag
			, log_pushdata
			, ipt::redundancy(ipt::load_cluster_cfg(cfg))
			, target_list);

	}

}
