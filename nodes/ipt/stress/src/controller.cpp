/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/sender.h"
#include "tasks/receiver.h"
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
#include <cyng/sys/process.h>
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
	void boot_up(cyng::async::mux&, cyng::logging::log_ptr, cyng::vector_t const&, cyng::tuple_t const&);

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
					//	initialize logger
					//
#if BOOST_OS_LINUX
					auto logger = cyng::logging::make_sys_logger("ipt:stress", true);
#else
					auto logger = cyng::logging::make_console_logger(mux.get_io_service(), "ipt:stress");
#endif

					CYNG_LOG_TRACE(logger, cyng::io::to_str(config));
					CYNG_LOG_INFO(logger, "pool size: " << this->pool_size_);

					//
					//	start application
					//
					cyng::vector_t vec;
					vec = cyng::value_cast(config, vec);
					BOOST_ASSERT_MSG(!vec.empty(), "invalid configuration");
					shutdown = vec.empty()
						? true
						: start(mux, logger, vec[0]);

					//
					//	print uptime
					//
					const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tp_start);
					CYNG_LOG_INFO(logger, "uptime " << cyng::io::to_str(cyng::make_object(duration)));
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

			//
			//	reconnect to master on different times
			//
			boost::random::mt19937 rng_;
            rng_.seed(std::time(nullptr));
			boost::random::uniform_int_distribution<int> monitor_dist(10, 120);


			const auto conf = cyng::vector_factory({
				cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", uidgen())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))

				, cyng::param_factory("ipt", cyng::vector_factory({
					cyng::tuple_factory(
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "26862"),
						cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::param_factory("scrambled", true),
						cyng::param_factory("monitor", monitor_dist(rng_))),	//	seconds
					cyng::tuple_factory(
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "26863"),
						cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::param_factory("scrambled", false),
						cyng::param_factory("monitor", monitor_dist(rng_)))
					}))
				//	generate a data set
				, cyng::param_factory("stress", cyng::tuple_factory(
						//cyng::param_factory("config", (pwd / "ipt-stress.xml").string()),
						cyng::param_factory("max-count", 4),	//	x 2
						cyng::param_factory("auto-generate", false)
						)
					)
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
			::OutputDebugString("An instance of the [e350] service is already running.");
			break;
		case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
			//
			//	The service process could not connect to the service controller.
			//	Typical error message, when running in console mode.
			//
			::OutputDebugString("***Error 1063: The [e350] service process could not connect to the service controller.");
			std::cerr
				<< "***Error 1063: The [e350] service could not connect to the service controller."
				<< std::endl
				;
			break;
		case ERROR_SERVICE_NOT_IN_EXE:
			//	The executable program that this service is configured to run in does not implement the service.
			::OutputDebugString("The [e350] service is configured to run in does not implement the service.");
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

#if BOOST_OS_LINUX
        const boost::filesystem::path log_dir = cyng::value_cast<std::string>(dom.get("log-dir"), ".");
        const auto pid = (log_dir / boost::uuids::to_string(tag)).replace_extension(".pid").string();
        std::fstream fout(pid, std::ios::trunc | std::ios::out);
        if (fout.is_open())
        {
            CYNG_LOG_INFO(logger, "write PID file: " << pid);
            fout << cyng::sys::get_process_id();
            fout.close();
        }
        else
        {
            CYNG_LOG_ERROR(logger, "could not write PID file: " << pid);
        }
#endif

		//
		//	boot up test
		//
		cyng::vector_t tmp_ipt;
		cyng::tuple_t tmp_stress;
		boot_up(mux, logger, cyng::value_cast(dom.get("ipt"), tmp_ipt), cyng::value_cast(dom.get("stress"), tmp_stress));

		//
		//	wait for system signals
		//
		const bool shutdown = wait(logger);

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


	void boot_up(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::vector_t const& cfg_ipt, cyng::tuple_t const& cfg_stress)
	{
		auto dom = cyng::make_reader(cfg_stress);
		auto count = cyng::value_cast<int>(dom.get("max-count"), 4);
		CYNG_LOG_INFO(logger, "boot up " << count * 2 << " tasks");

		ipt::master_config_t cfg = ipt::load_cluster_cfg(cfg_ipt);
		BOOST_ASSERT_MSG(!cfg.empty(), "no IP-T configuration available");
		std::stringstream ss;
		ss.fill('0');
		for (std::size_t idx = 0u; idx < count; idx++)
		{
			ss.str("");
			ss
				<< "sender-"
				<< std::setw(4)
				<< idx;

			std::for_each(cfg.begin(), cfg.end(), [&](ipt::master_record& rec) {
				const_cast<std::string&>(rec.account_) = ss.str();
				const_cast<std::string&>(rec.pwd_) = ss.str();
			});

			auto sender = cyng::async::start_task_delayed<ipt::sender>(mux
				, std::chrono::milliseconds(idx + 1)
				, logger
				, cfg);

			if (sender.second)
			{
				ss.str("");
				ss
					<< "receiver-"
					<< std::setw(4)
					<< idx;

				std::for_each(cfg.begin(), cfg.end(), [&](ipt::master_record& rec) {
					const_cast<std::string&>(rec.account_) = ss.str();
					const_cast<std::string&>(rec.pwd_) = ss.str();
				});

				auto receiver = cyng::async::start_task_delayed<ipt::receiver>(mux
					, std::chrono::milliseconds(idx + 2)
					, logger
					, cfg
					, sender.first);
				if (!receiver.second)
				{
					CYNG_LOG_FATAL(logger, "could not start IP-T receiver #" << idx);
				}
			}
			else
			{
				CYNG_LOG_FATAL(logger, "could not start IP-T sender #" << idx);
			}
		}
	}
}
