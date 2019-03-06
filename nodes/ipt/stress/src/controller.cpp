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
#include <cyng/rnd.h>
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
#include <boost/numeric/conversion/cast.hpp>
#include <fstream>

namespace node 
{
	//
	//	forward declaration(s):
	//
	bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::object);
	bool wait(cyng::logging::log_ptr logger);
	void boot_up(cyng::async::mux&, cyng::logging::log_ptr, cyng::vector_t const&, cyng::tuple_t const&);
	std::vector<std::size_t> start_sender(cyng::async::mux&
		, cyng::logging::log_ptr
		, ipt::master_config_t const& cfg
		, std::size_t count
		, std::string const& prefix
		, int packet_size_min
		, int packet_size_max
		, int delay
		, int retries);

	void start_receiver(cyng::async::mux&
		, cyng::logging::log_ptr
		, ipt::master_config_t const& cfg
		, std::vector<std::size_t> const&
		, std::string const& prefix
		, int rec_limit
		, int packet_size_min
		, int packet_size_max
		, int delay
		, int retries);

	std::vector<std::size_t> start_sender_collision(cyng::async::mux&
		, cyng::logging::log_ptr
		, ipt::master_config_t const& cfg
		, std::size_t count
		, std::string const& prefix
		, int packet_size_min
		, int packet_size_max
		, int delay
		, int retries);

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

					if (vec.empty()) {
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
						auto logger = cyng::logging::make_sys_logger("ipt:stress", true);
#else
						const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
						auto dom = cyng::make_reader(vec[0]);
						const boost::filesystem::path log_dir = cyng::value_cast(dom.get("log-dir"), tmp.string());

						auto logger = (console)
							? cyng::logging::make_console_logger(mux.get_io_service(), "ipt:stress")
							: cyng::logging::make_file_logger(mux.get_io_service(), (log_dir / "ipt-stress.log"))
							;
#ifdef _DEBUG
						if (!console) {
							std::cout << "log file see: " << (log_dir / "ipt-stress.log") << std::endl;
						}
#endif
#endif

						CYNG_LOG_TRACE(logger, cyng::io::to_str(config));
						CYNG_LOG_INFO(logger, "pool size: " << this->pool_size_);

						//
						//	start application
						//
						shutdown = start(mux, logger, vec[0]);

						//
						//	print uptime
						//
						const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tp_start);
						CYNG_LOG_INFO(logger, "ipt:stress uptime " << cyng::io::to_str(cyng::make_object(duration)));
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

			//
			//	generate even distributed integers
			//	reconnect to master on different times
			//
			cyng::crypto::rnd_num<int> rng(10, 120);

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
						cyng::param_factory("monitor", rng())),	//	seconds
					cyng::tuple_factory(
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "26863"),
						cyng::param_factory("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::param_factory("scrambled", false),
						cyng::param_factory("monitor", rng()))
					}))
				//	generate a data set
				, cyng::param_factory("stress", cyng::tuple_factory(
						//cyng::param_factory("config", (pwd / "ipt-stress.xml").string()),
						cyng::param_factory("max-count", 4),	//	x 2
						cyng::param_factory("mode", "same"),	//	same, distinct, collision
						cyng::param_factory("mode-2", "distinct"),	//	same, distinct, collision
						cyng::param_factory("mode-3", "collision"),	//	same, distinct, collision
						cyng::param_factory("prefix-sender", "sender"),
						cyng::param_factory("prefix-receiver", "receiver"),
						cyng::param_factory("size-min", 512),	//	minimal packet size
						cyng::param_factory("size-max", 1024),	//	maximal packet size
						cyng::param_factory("delay", 200),	//	delay between send operations in milliseconds
						cyng::param_factory("receiver-limit", 0x10000),	//	cut connection after receiving that much data
						cyng::param_factory("connection-open-retries", 1)	//	be carefull! value one is highly recommended
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

		const auto mode = cyng::value_cast<std::string>(dom.get("mode"), "both");
		CYNG_LOG_INFO(logger, "mode " << mode);

		const auto prefix_sender = cyng::value_cast<std::string>(dom.get("prefix-sender"), "sender");
		const auto prefix_receiver = cyng::value_cast<std::string>(dom.get("prefix-receiver"), "receiver");

		const auto rec_limit = cyng::value_cast<int>(dom.get("receiver-limit"), 0x1000);
		const auto packet_size_min = cyng::value_cast<int>(dom.get("size-min"), 512);
		const auto packet_size_max = cyng::value_cast<int>(dom.get("size-max"), 1024);
		BOOST_ASSERT_MSG(packet_size_min <= packet_size_max, "invalid packet size range");

		const auto delay = cyng::value_cast<int>(dom.get("delay"), 400);
		const auto retries = cyng::value_cast<int>(dom.get("connection-open-retries"), 1);
		

		if (boost::algorithm::equals("same", mode)) {

			const auto count = cyng::value_cast<int>(dom.get("max-count"), 4);
			CYNG_LOG_INFO(logger, "boot up " << count * 2 << " tasks in mode " << mode);

			const ipt::master_config_t cfg = ipt::load_cluster_cfg(cfg_ipt);
			BOOST_ASSERT_MSG(!cfg.empty(), "no IP-T configuration available");

			//
			//	connecting to the same IP-T master
			//
			auto vec = start_sender(mux, logger, cfg, count, prefix_sender, packet_size_min, packet_size_max, delay, retries);
			start_receiver(mux, logger, cfg, vec, prefix_receiver, rec_limit, packet_size_min, packet_size_max, delay, retries);
		}
		else if (boost::algorithm::equals("distinct", mode)) {

			const auto count = cyng::value_cast<int>(dom.get("max-count"), 4);
			CYNG_LOG_INFO(logger, "boot up " << count << " sender tasks in mode " << mode);

			//	std::vector<master_record>
			const ipt::master_config_t cfg = ipt::load_cluster_cfg(cfg_ipt);
			BOOST_ASSERT_MSG(cfg.size() > 1, "more IP-T configurations required");

			//
			//	build alternate configurations
			//
			ipt::master_config_t cfg_sender, cfg_receiver;
			for (std::size_t idx = 0; idx < cfg.size(); ++idx) {
				if ((idx % 2) == 0) {
					cfg_sender.push_back(cfg.at(idx));
				}
				else {
					cfg_receiver.push_back(cfg.at(idx));
				}
			}

			//
			//	connecting to different IP-T masters
			//
			auto vec = start_sender(mux, logger, cfg_sender, count, prefix_sender, packet_size_min, packet_size_max, delay, retries);
			start_receiver(mux, logger, cfg_receiver, vec, prefix_receiver, rec_limit, packet_size_min, packet_size_max, delay, retries);
		}
		else if (boost::algorithm::equals("collision", mode)) {

			const auto count = cyng::value_cast<int>(dom.get("max-count"), 4);
			CYNG_LOG_INFO(logger, "boot up " << count << " tasks in mode " << mode);

			const ipt::master_config_t cfg = ipt::load_cluster_cfg(cfg_ipt);
			BOOST_ASSERT_MSG(!cfg.empty(), "no IP-T configuration available");

			//
			//	connecting to the same IP-T master with the same credentials
			//
			auto vec = start_sender_collision(mux, logger, cfg, count, prefix_sender, packet_size_min, packet_size_max, delay, retries);
			//start_receiver(mux, logger, cfg, vec, prefix_receiver, rec_limit, packet_size_min, packet_size_max, delay, retries);
		}
		else {
			CYNG_LOG_ERROR(logger, "unknown mode " << mode << " - use [same|distinct|collision]");
		}


	}

	std::vector<std::size_t> start_sender(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, ipt::master_config_t const& cfg
		, std::size_t count
		, std::string const& prefix
		, int packet_size_min
		, int packet_size_max
		, int delay
		, int retries)
	{
		std::vector<std::size_t> vec;
		vec.reserve(count);

		std::stringstream ss;
		ss.fill('0');
		for (std::size_t idx = 0u; idx < count; idx++)
		{
			ss.str("");
			ss
				<< prefix
				<< '-'
				<< std::setw(4)
				<< idx;

			//
			//	prepare IP-T configuration
			//
			std::for_each(cfg.begin(), cfg.end(), [&](ipt::master_record const& rec) {
				const_cast<std::string&>(rec.account_) = ss.str();
				const_cast<std::string&>(rec.pwd_) = ss.str();
			});

			auto r = cyng::async::start_task_delayed<ipt::sender>(mux
				, std::chrono::milliseconds(idx + 1)
				, logger
				, ipt::redundancy(cfg)
				, boost::numeric_cast<std::size_t>((packet_size_min < 1) ? 1 : packet_size_min)
				, boost::numeric_cast<std::size_t>((packet_size_max < packet_size_min) ? packet_size_min : packet_size_max)
				, std::chrono::milliseconds(delay)
				, boost::numeric_cast<std::size_t>((retries < 1) ? 1 : retries));

			if (!r.second) {
				CYNG_LOG_FATAL(logger, "could not start IP-T sender #" << idx);
			}
			else {
				vec.push_back(r.first);
			}
		}

		return vec;
	}

	void start_receiver(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, ipt::master_config_t const& cfg
		, std::vector<std::size_t> const& st
		, std::string const& prefix
		, int rec_limit, int packet_size_min
		, int packet_size_max
		, int delay
		, int retries)
	{
		std::stringstream ss;
		ss.fill('0');
		for (std::size_t idx = 0u; idx < st.size(); idx++)
		{
			ss.str("");
			ss
				<< prefix
				<< '-'
				<< std::setw(4)
				<< idx;

			//
			//	prepare IP-T configuration
			//
			std::for_each(cfg.begin(), cfg.end(), [&](ipt::master_record const& rec) {
				const_cast<std::string&>(rec.account_) = ss.str();
				const_cast<std::string&>(rec.pwd_) = ss.str();
			});

			auto r = cyng::async::start_task_delayed<ipt::receiver>(mux
				, std::chrono::seconds((idx + 1) * 10)
				, logger
				, cfg
				, st
				, idx
				, boost::numeric_cast<std::size_t>((rec_limit < 1) ? 1 : rec_limit)
				, boost::numeric_cast<std::size_t>((packet_size_min < 1) ? 1 : packet_size_min)
				, boost::numeric_cast<std::size_t>((packet_size_max < packet_size_min) ? packet_size_min : packet_size_max)
				, std::chrono::milliseconds(delay)
				, boost::numeric_cast<std::size_t>((retries < 1) ? 1 : retries));

			if (!r.second) {
				CYNG_LOG_FATAL(logger, "could not start IP-T receiver #" << idx);
			}
		}
	}

	std::vector<std::size_t> start_sender_collision(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, ipt::master_config_t const& cfg
		, std::size_t count
		, std::string const& prefix
		, int packet_size_min
		, int packet_size_max
		, int delay
		, int retries)
	{
		std::vector<std::size_t> vec;
		vec.reserve(count);

		std::stringstream ss;
		ss.fill('0');
		ss
			<< prefix
			<< "-collision"
			;
		const auto name = ss.str();

		for (std::size_t idx = 0u; idx < count; idx++)
		{
			//
			//	prepare IP-T configuration
			//
			std::for_each(cfg.begin(), cfg.end(), [&](ipt::master_record const& rec) {
				const_cast<std::string&>(rec.account_) = name;
				const_cast<std::string&>(rec.pwd_) = name;
			});

			auto r = cyng::async::start_task_delayed<ipt::sender>(mux
				, std::chrono::milliseconds(idx)
				, logger
				, ipt::redundancy(cfg)
				, boost::numeric_cast<std::size_t>((packet_size_min < 1) ? 1 : packet_size_min)
				, boost::numeric_cast<std::size_t>((packet_size_max < packet_size_min) ? packet_size_min : packet_size_max)
				, std::chrono::milliseconds(delay)
				, boost::numeric_cast<std::size_t>((retries < 1) ? 1 : retries));

			if (!r.second) {
				CYNG_LOG_FATAL(logger, "could not start IP-T sender #" << idx);
			}
			else {
				vec.push_back(r.first);
			}
		}

		return vec;
	}

}
