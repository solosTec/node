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
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/async/signal_handler.h>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/json.h>
#include <cyng/value_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/vector_cast.hpp>
#if BOOST_OS_WINDOWS
#include <cyng/scm/service.hpp>
#endif
#if BOOST_OS_LINUX
#include "../../../write_pid.h"
#endif
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

namespace node 
{
	//
	//	forward declaration(s):
	//
	bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::object);
	bool wait(cyng::logging::log_ptr logger);

	/**
	 * start ipt client
	 */
	std::size_t join_network(cyng::async::mux&, cyng::logging::log_ptr, cyng::vector_t const&, cyng::param_map_t);

	/**
	 * start all data consumer tasks
	 */
	std::vector<std::size_t> connect_data_store(cyng::async::mux&
		, cyng::logging::log_ptr
		, std::vector<std::string> const&
		, std::size_t ntid	//	network task id
		, cyng::object cfg);

	controller::controller(unsigned int pool_size, std::string const& json_path)
	: pool_size_(pool_size)
	, json_path_(json_path)
	{}

	int controller::run(bool console)
	{
		//
		//	to calculate uptime
		//
		const std::chrono::system_clock::time_point tp_start = std::chrono::system_clock::now();

		//
		//	controller loop
		//
		try
		{
			//
			//	controller loop
			//
			bool shutdown{ false };
			while (!shutdown)
			{
				//
				//	establish I/O context
				//
				cyng::async::mux mux{ this->pool_size_ };

				//
				//	read configuration file
				//
				cyng::object config = cyng::json::read_file(json_path_);
				if (config)
				{
					//
					//	start application
					//
					cyng::vector_t vec;
					vec = cyng::value_cast(config, vec);

					if (vec.empty())
					{
						std::cerr
							<< "use option -D to generate a configuration file"
							<< std::endl;
						shutdown = true;
					}
					else
					{
						//
						//	initialize logger
						//
#if BOOST_OS_LINUX
						auto logger = cyng::logging::make_sys_logger("ipt:store", true);
#else
						const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
						auto dom = cyng::make_reader(vec[0]);
						const boost::filesystem::path log_dir = cyng::value_cast(dom.get("log-dir"), tmp.string());

						auto logger = (console)
							? cyng::logging::make_console_logger(mux.get_io_service(), "ipt:store")
							: cyng::logging::make_file_logger(mux.get_io_service(), (log_dir / "ipt-store.log"))
							;
#endif

						CYNG_LOG_TRACE(logger, cyng::io::to_str(config));
						CYNG_LOG_INFO(logger, "pool size: " << this->pool_size_);

						//
						//	start
						//
						shutdown = start(mux, logger, vec[0]);

						//
						//	print uptime
						//
						const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tp_start);
						CYNG_LOG_INFO(logger, "uptime " << cyng::io::to_str(cyng::make_object(duration)));
					}
				}
				else
				{
					// 					CYNG_LOG_FATAL(logger, "no configuration data");
					std::cout
						<< "use option -D to generate a configuration file"
						<< std::endl;

					//
					//	no configuration found
					//
					shutdown = true;
				}

				//
				//	shutdown scheduler
				//
				mux.shutdown();
			}

			return EXIT_SUCCESS;
		}
		catch (std::exception& ex)
		{
			std::cerr
				<< ex.what()
				<< std::endl;
		}

		return EXIT_FAILURE;
	}

	int controller::create_config() const
	{
		std::fstream fout(json_path_, std::ios::trunc | std::ios::out);
		if (fout.is_open())
		{
			std::cout
				<< "write to file "
				<< json_path_
				<< std::endl;
			//
			//	get default values
			//
			const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
			const boost::filesystem::path pwd = boost::filesystem::current_path();
			boost::uuids::random_generator uidgen;

			const auto conf = cyng::vector_factory({
				cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", uidgen())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
				
				, cyng::param_factory("log-pushdata", false)	//	log file for each channel
				, cyng::param_factory("output", cyng::vector_factory({"ALL:BIN"}))	//	options are XML, JSON, DB, BIN, ...

				, cyng::param_factory("SML:DB", cyng::tuple_factory(
					cyng::param_factory("type", "SQLite"),
					cyng::param_factory("file-name", (pwd / "store.database").string()),
					cyng::param_factory("busy-timeout", 12),		//	seconds
					cyng::param_factory("watchdog", 30),	//	for database connection
					cyng::param_factory("pool-size", 1),	//	no pooling for SQLite
					cyng::param_factory("db-schema", NODE_SUFFIX),		//	use "v4.0" for compatibility to version 4.x
					cyng::param_factory("period", 12)	//	seconds
				))
				, cyng::param_factory("IEC:DB", cyng::tuple_factory(
					cyng::param_factory("type", "SQLite"),
					cyng::param_factory("file-name", (pwd / "store.database").string()),
					cyng::param_factory("busy-timeout", 12),		//	seconds
					cyng::param_factory("watchdog", 30),	//	for database connection
					cyng::param_factory("pool-size", 1),	//	no pooling for SQLite
					cyng::param_factory("db-schema", NODE_SUFFIX),
					cyng::param_factory("period", 12),	//	seconds
					cyng::param_factory("ignore-null", false)	//	don't write values equal 0
				))
				, cyng::param_factory("SML:XML", cyng::tuple_factory(
					cyng::param_factory("root-dir", (pwd / "xml").string()),
					cyng::param_factory("root-name", "SML"),
					cyng::param_factory("endcoding", "UTF-8"),
					cyng::param_factory("period", 16)	//	seconds
				))
				, cyng::param_factory("SML:JSON", cyng::tuple_factory(
					cyng::param_factory("root-dir", (pwd / "json").string()),
					cyng::param_factory("prefix", "smf"),
					cyng::param_factory("suffix", "json"),
					cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR).to_double()),
					cyng::param_factory("period", 16)	//	seconds
				))
				, cyng::param_factory("SML:ABL", cyng::tuple_factory(
					cyng::param_factory("root-dir", (pwd / "abl").string()),
					cyng::param_factory("prefix", "smf"),
					cyng::param_factory("suffix", "abl"),
					cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR).to_double())
				))
				, cyng::param_factory("ALL:BIN", cyng::tuple_factory(
					cyng::param_factory("root-dir", (pwd / "sml").string()),
					cyng::param_factory("prefix", "smf"),
					cyng::param_factory("suffix", "sml"),
					cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR).to_double()),
					cyng::param_factory("period", 16)	//	seconds
				))
				, cyng::param_factory("SML:LOG", cyng::tuple_factory(
					cyng::param_factory("root-dir", (pwd / "log").string()),
					cyng::param_factory("prefix", "sml"),
					cyng::param_factory("suffix", "log"),
					cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR).to_double()),
					cyng::param_factory("period", 16)	//	seconds
				))
				, cyng::param_factory("IEC:LOG", cyng::tuple_factory(
					cyng::param_factory("root-dir", (pwd / "log").string()),
					cyng::param_factory("prefix", "iec"),
					cyng::param_factory("suffix", "log"),
					cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)),
					cyng::param_factory("period", 16)	//	seconds
				))
				, cyng::param_factory("SML:CSV", cyng::tuple_factory(
					cyng::param_factory("root-dir", (pwd / "csv").string()),
					cyng::param_factory("prefix", "smf"),
					cyng::param_factory("suffix", "csv"),
					cyng::param_factory("header", true),
					cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)),
					cyng::param_factory("period", 16)	//	seconds
				))
				, cyng::param_factory("ipt", cyng::vector_factory({
					cyng::tuple_factory(
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "26862"),
						cyng::param_factory("account", "data-store"),
						cyng::param_factory("pwd", "to-define"),
						cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::param_factory("scrambled", true),
						cyng::param_factory("monitor", 57)),	//	seconds
					cyng::tuple_factory(
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "26863"),
						cyng::param_factory("account", "data-store"),
						cyng::param_factory("pwd", "to-define"),
						cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::param_factory("scrambled", false),
						cyng::param_factory("monitor", 57))
					}))
					, cyng::param_factory("targets", cyng::vector_factory({ 
						cyng::param_factory("data.sink.sml", "SML"),
						cyng::param_factory("data.sink.iec", "IEC") }))	//	list of targets and there data type
					)
				});

			cyng::json::write(std::cout, cyng::make_object(conf));
			std::cout << std::endl;
			cyng::json::write(fout, cyng::make_object(conf));
			return EXIT_SUCCESS;
		}
		else
		{
			std::cerr
				<< "unable to open file "
				<< json_path_
				<< std::endl;

		}
		return EXIT_FAILURE;
	}

	int controller::init_db()
	{
		//
		//	read configuration file
		//
		cyng::object config = cyng::json::read_file(json_path_);

		cyng::vector_t vec;
		vec = cyng::value_cast(config, vec);
		BOOST_ASSERT_MSG(!vec.empty(), "invalid configuration");

		if (!vec.empty())
		{
			auto dom = cyng::make_reader(vec[0]);
			cyng::tuple_t tpl;
			return (EXIT_SUCCESS == sml_db_consumer::init_db(cyng::value_cast(dom.get("SML:DB"), tpl)))
				&& (EXIT_SUCCESS == iec_db_consumer::init_db(cyng::value_cast(dom.get("IEC:DB"), tpl)));
		}
		return EXIT_FAILURE;
	}

#if BOOST_OS_WINDOWS
	int controller::run_as_service(controller&& ctrl, std::string const& srv_name)
	{
		//
		//	define service type
		//
		typedef service< controller >	service_type;

		//
		//	create service
		//
		::OutputDebugString(srv_name.c_str());
		service_type srv(std::move(ctrl), srv_name);

		//
		//	starts dispatcher and calls service main() function 
		//
		const DWORD r = srv.run();
		switch (r)
		{
		case ERROR_SERVICE_ALREADY_RUNNING:
			//	An instance of the service is already running.
			::OutputDebugString("An instance of the [ipt:store] service is already running.");
			break;
		case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
			//
			//	The service process could not connect to the service controller.
			//	Typical error message, when running in console mode.
			//
			::OutputDebugString("***Error 1063: The [ipt:store] service process could not connect to the service controller.");
			std::cerr
				<< "***Error 1063: The [ipt:store] service could not connect to the service controller."
				<< std::endl
				;
			break;
		case ERROR_SERVICE_NOT_IN_EXE:
			//	The executable program that this service is configured to run in does not implement the service.
			::OutputDebugString("The [ipt:store] service is configured to run in does not implement the service.");
			break;
		default:
		{
			std::stringstream ss;
			ss
				<< '['
				<< srv_name
				<< "] service dispatcher stopped "
				<< r;
			const std::string msg = ss.str();
			::OutputDebugString(msg.c_str());
		}
		break;
		}


		return EXIT_SUCCESS;
	}

	void controller::control_handler(DWORD sig)
	{
		//	forward signal to shutdown manager
		cyng::forward_signal(sig);
	}

#endif

	bool start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::object cfg)
	{
		CYNG_LOG_TRACE(logger, cyng::dom_counter(cfg) << " configuration nodes found");
		auto dom = cyng::make_reader(cfg);

		boost::uuids::random_generator uidgen;
		const auto tag = cyng::value_cast<boost::uuids::uuid>(dom.get("tag"), uidgen());

		//
		//	apply severity threshold
		//
		logger->set_severity(cyng::logging::to_severity(cyng::value_cast<std::string>(dom.get("log-level"), "INFO")));

        //
        //  control logging of push data
        //
		const auto log_pushdata = cyng::value_cast(dom.get("log-pushdata"), false);
		
#if BOOST_OS_LINUX
        const boost::filesystem::path log_dir = cyng::value_cast<std::string>(dom.get("log-dir"), ".");
        write_pid(log_dir, tag);
#endif

		//
		//	connect to ipt master
		//
		cyng::vector_t tmp;
		auto ntid = join_network(mux
			, logger
			, cyng::value_cast(dom.get("ipt"), tmp)
			, cyng::to_param_map(cyng::value_cast(dom.get("targets"), tmp)));

		//
		//	get configuration type
		//
		const auto config_types = cyng::vector_cast<std::string>(dom.get("output"), "");

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
		const bool shutdown = wait(logger);

		//
		//	close acceptor
		//
		CYNG_LOG_INFO(logger, "close acceptor");
		//srv.close();

		//
		//	stop all connections
		//
		CYNG_LOG_INFO(logger, "stop all connections");

		//
		//	stop all tasks
		//
		CYNG_LOG_INFO(logger, "stop all tasks");
		mux.stop();

		return shutdown;
	}

	bool wait(cyng::logging::log_ptr logger)
	{
		//
		//	wait for system signals
		//
		bool shutdown = false;
		cyng::signal_mgr signal;
		switch (signal.wait())
		{
#if BOOST_OS_WINDOWS
		case CTRL_BREAK_EVENT:
#else
		case SIGHUP:
#endif
			CYNG_LOG_INFO(logger, "SIGHUP received");
			break;
		default:
			CYNG_LOG_WARNING(logger, "SIGINT received");
			shutdown = true;
			break;
		}
		return shutdown;
	}

	/**
	 * start all data consumer tasks
	 */
	std::vector<std::size_t> connect_data_store(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, std::vector<std::string> const& config_types
		, std::size_t ntid	//	network task id
		, cyng::object cfg)
	{
		std::vector<std::size_t> tsks;
		cyng::tuple_t tpl;
		auto dom = cyng::make_reader(cfg);
		const boost::filesystem::path pwd = boost::filesystem::current_path();

		for (const auto& config_type : config_types)
		{
			if (boost::algorithm::iequals(config_type, "SML:DB"))
			{
				CYNG_LOG_INFO(logger, "start database adapter for SML protocol");

				tpl = cyng::value_cast(dom.get(config_type), tpl);

				tsks.push_back(cyng::async::start_task_delayed<sml_db_consumer>(mux
					, std::chrono::seconds(1)
					, logger
					, ntid
					, cyng::to_param_map(tpl)).first);
			}
			else if (boost::algorithm::iequals(config_type, "IEC:DB"))
			{
				CYNG_LOG_INFO(logger, "start database adapter for IEC 62056-21 protocol (readout mode)");

				tpl = cyng::value_cast(dom.get(config_type), tpl);

				tsks.push_back(cyng::async::start_task_delayed<iec_db_consumer>(mux
					, std::chrono::seconds(1)
					, logger
					, ntid
					, cyng::to_param_map(tpl)).first);
			}
			else if (boost::algorithm::iequals(config_type, "SML:XML"))
			{
				CYNG_LOG_INFO(logger, "start SML:XML consumer");

				tpl = cyng::value_cast(dom.get(config_type), tpl);

				boost::filesystem::path root_dir = cyng::value_cast(dom[config_type].get("root-dir"), (pwd / "xml").string());
				auto root_name = cyng::value_cast<std::string>(dom[config_type].get("root-name"), "SML");
				auto encoding = cyng::value_cast<std::string>(dom[config_type].get("endcoding"), "UTF-8");
				auto period = cyng::value_cast(dom[config_type].get("period"), 16);	//	seconds

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

				tpl = cyng::value_cast(dom.get(config_type), tpl);

				boost::filesystem::path root_dir = cyng::value_cast(dom[config_type].get("root-dir"), (pwd / "json").string());
				auto prefix = cyng::value_cast<std::string>(dom[config_type].get("prefix"), "sml");
				auto suffix = cyng::value_cast<std::string>(dom[config_type].get("suffix"), "json");

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

				tpl = cyng::value_cast(dom.get(config_type), tpl);

				boost::filesystem::path root_dir = cyng::value_cast(dom[config_type].get("root-dir"), (pwd / "abl").string());
				auto prefix = cyng::value_cast<std::string>(dom[config_type].get("prefix"), "sml");
				auto suffix = cyng::value_cast<std::string>(dom[config_type].get("suffix"), "abl");

				tsks.push_back(cyng::async::start_task_delayed<sml_abl_consumer>(mux
					, std::chrono::seconds(1)
					, logger
					, ntid
					, cyng::to_param_map(tpl)).first);
			}
			else if (boost::algorithm::iequals(config_type, "ALL:BIN"))
			{
				CYNG_LOG_INFO(logger, "start ALL:BIN consumer");

				auto root_dir = cyng::value_cast(dom[config_type].get("root-dir"), (pwd / "sml").string());
				auto prefix = cyng::value_cast<std::string>(dom[config_type].get("prefix"), "sml");
				auto suffix = cyng::value_cast<std::string>(dom[config_type].get("suffix"), "sml");
				auto period = cyng::value_cast(dom[config_type].get("period"), 16);	//	seconds

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

				tpl = cyng::value_cast(dom.get(config_type), tpl);

				tsks.push_back(cyng::async::start_task_delayed<sml_log_consumer>(mux
					, std::chrono::seconds(1)
					, logger
					, ntid
					, cyng::to_param_map(tpl)).first);
			}
			else if (boost::algorithm::iequals(config_type, "SML:CSV"))
			{
				CYNG_LOG_INFO(logger, "start SML:CSV storage");

				auto version = cyng::value_cast(dom[config_type].get("version"), cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR).to_double());
				boost::filesystem::path root_dir = cyng::value_cast(dom[config_type].get("root-dir"), (pwd / "csv").string());
				auto prefix = cyng::value_cast<std::string>(dom[config_type].get("prefix"), "smf");
				auto suffix = cyng::value_cast<std::string>(dom[config_type].get("suffix"), "csv");
				auto header = cyng::value_cast(dom[config_type].get("header"), true);
				auto period = cyng::value_cast(dom[config_type].get("period"), 16);	//	seconds

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

		return cyng::async::start_task_detached<ipt::network>(mux
			, logger
			, ipt::load_cluster_cfg(cfg)
			, target_list);

	}

}
