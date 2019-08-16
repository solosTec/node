/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include <smf/shared/ctl.h>
#include <NODE_project_info.h>
#include <cyng/async/mux.h>
#include <cyng/async/signal_handler.h>
#include <cyng/json.h>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/factory/set_factory.h>

#if BOOST_OS_LINUX
#include <smf/shared/write_pid.h>
#endif

#if BOOST_OS_WINDOWS
#include <cyng/scm/service.hpp>
#endif

#include <boost/filesystem.hpp>

namespace node
{

	ctl::ctl(unsigned int pool_size, std::string const& json_path, std::string node_name)
		: pool_size_(pool_size)
		, json_path_(json_path)
		, node_name_(node_name)
		, uidgen_()
	{}

	int ctl::run(bool console)
	{
		//
		//	to calculate uptime
		//
		std::chrono::system_clock::time_point const tp_start = std::chrono::system_clock::now();

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
					if (!vec.empty())
					{
#if BOOST_OS_LINUX
						auto logger = cyng::logging::make_sys_logger(node_name_, true);
#else
						const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
						auto dom = cyng::make_reader(vec[0]);
						const boost::filesystem::path log_dir = cyng::value_cast(dom.get("log-dir"), tmp.string());

						auto logger = (console)
							? cyng::logging::make_console_logger(mux.get_io_service(), node_name_)
							: cyng::logging::make_file_logger(mux.get_io_service(), (log_dir / get_logfile_name()))
							;
#ifdef _DEBUG
						if (!console) {
							std::cout << "log file see: " << (log_dir / get_logfile_name()) << std::endl;
						}
#endif
#endif

						CYNG_LOG_TRACE(logger, cyng::io::to_str(config));
						CYNG_LOG_INFO(logger, "pool size: " << this->pool_size_);

						shutdown = pre_start(mux, logger, vec.at(0));

						//
						//	stop all tasks
						//
						CYNG_LOG_INFO(logger, "stop all tasks");
						mux.stop(std::chrono::milliseconds(100), 10);

						//
						//	print uptime
						//
						const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tp_start);
						CYNG_LOG_INFO(logger, "uptime " << cyng::to_str(duration));

					}
					else
					{
						std::cerr
							<< "use option -D to generate a configuration file"
							<< std::endl;

						shutdown = true;
					}
				}
				else
				{
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
			// 			CYNG_LOG_FATAL(logger, ex.what());
			std::cerr
				<< ex.what()
				<< std::endl;
		}
		return EXIT_FAILURE;
	}

	bool ctl::start(cyng::async::mux&, cyng::logging::log_ptr, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag)
	{
		return false;
	}

	bool ctl::pre_start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::object const& cfg)
	{
		//
		//	make configuration reader and extract some basic data
		//
		auto const dom = cyng::make_reader(cfg);
		auto const tag = cyng::value_cast<boost::uuids::uuid>(dom.get("tag"), uidgen_());

		//
		//	apply severity threshold
		//
		logger->set_severity(cyng::logging::to_severity(cyng::value_cast<std::string>(dom.get("log-level"), "INFO")));

#if BOOST_OS_LINUX
		const boost::filesystem::path log_dir = cyng::value_cast<std::string>(dom.get("log-dir"), ".");
		write_pid(log_dir, tag);
#endif

		return start(mux, logger, dom, tag);
	}

	int ctl::create_config() const
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
			auto obj = cyng::make_object(create_config(fout, boost::filesystem::temp_directory_path(), boost::filesystem::current_path()));
			cyng::json::write(std::cout, obj);
			std::cout << std::endl;
			cyng::json::write(fout, obj);

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

	cyng::vector_t ctl::create_config(std::fstream&, boost::filesystem::path&& tmp, boost::filesystem::path&& cwd) const
	{
		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
			, cyng::param_factory("log-level", "INFO")
			, cyng::param_factory("tag", uidgen_())
			, cyng::param_factory("generated", std::chrono::system_clock::now())
			, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR)))
			});

	}

	std::string ctl::get_logfile_name() const
	{
		std::string logfile_name;
		std::transform(node_name_.begin(), node_name_.end(), std::back_inserter(logfile_name),
			[](unsigned char c) -> std::size_t { 

				switch (c) {
				case ':':
				case '\\':
				case '/': 
				case '*': 
				case '"':
				case '<':
				case '>':
				case '|':
					return '-';
				default:
					break;
				}
				return c; 
		});
		return logfile_name + ".log";
	}

#if BOOST_OS_WINDOWS
	int ctl::run_as_service(ctl&& ctrl, std::string const& srv_name)
	{
		//
		//	define service type
		//
		typedef service< ctl >	service_type;

		//
		//	messages
		//
		static std::string msg_01 = "startup service [" + srv_name + "]";
		static std::string msg_02 = "An instance of the [" + srv_name + "] service is already running";
		static std::string msg_03 = "***Error 1063: The [" + srv_name + "] service process could not connect to the service controller";
		static std::string msg_04 = "The [" + srv_name + "] service is configured to run in does not implement the service";

		//
		//	create service
		//
		::OutputDebugString(msg_01.c_str());
		service_type srv(std::move(ctrl), srv_name);

		//
		//	starts dispatcher and calls service main() function 
		//
		const DWORD r = srv.run();
		switch (r)
		{
		case ERROR_SERVICE_ALREADY_RUNNING:
			//	An instance of the service is already running.
			::OutputDebugString(msg_02.c_str());
			break;
		case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
			//
			//	The service process could not connect to the service controller.
			//	Typical error message, when running in console mode.
			//
			::OutputDebugString(msg_03.c_str());
			std::cerr
				<< msg_03
				<< std::endl
				;
			break;
		case ERROR_SERVICE_NOT_IN_EXE:
			//	The executable program that this service is configured to run in does not implement the service.
			::OutputDebugString(msg_04.c_str());
			break;
		default:
		{
			std::stringstream ss;
			ss
				<< '['
				<< srv_name
				<< "] service dispatcher stopped: "
				<< r;
			const std::string msg = ss.str();
			::OutputDebugString(msg.c_str());
		}
		break;
		}


		return EXIT_SUCCESS;
	}

	void ctl::control_handler(DWORD sig)
	{
		//	forward signal to shutdown manager
		cyng::forward_signal(sig);
	}

#endif

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

}