/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/controller_base.h>
#include <smf.h>

#include <cyng/obj/factory.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/task/controller.h>
#include <cyng/parse/json.h>
#include <cyng/log/log.h>
#include <cyng/log/record.h>

#include <fstream>
#include <iostream>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/asio.hpp>

namespace smf {
	namespace config {

		controller_base::controller_base(startup const& config)
			: config_(config)
			, uidgen_()
		{}

		bool controller_base::run_options(boost::program_options::variables_map& vars) {

			if (vars["default"].as< bool >())	{
				//	write default configuration
				write_config(create_default_config(std::chrono::system_clock::now()
					, std::filesystem::temp_directory_path()
					, std::filesystem::current_path()));
				return true;
			}
			if (vars["show"].as< bool >())	{
				//	show configuration
				print_configuration(std::cout);
				return true;
			}
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
			if (vars["service.enabled"].as< bool >()) {
				//	run as service 
				//::OutputDebugString("start master node");
				//const std::string srv_name = vm["service.name"].as< std::string >();
				//::OutputDebugString(srv_name.c_str());
				//return node::run_as_service(std::move(ctrl), srv_name);
			}
#endif

			return false;
		}

		void controller_base::write_config(cyng::vector_t&& vec) {

			//std::cout << cyng::to_string(vec) << std::endl;

			std::fstream fso(config_.json_path_, std::ios::trunc | std::ios::out);
			if (fso.is_open()) {

				std::cout
					<< "write to file "
					<< config_.json_path_
					<< std::endl;

				//
				//	get default values
				//
				if (vec.empty()) {
					std::cerr
						<< "** warning: configuration is empty"
						<< std::endl;
				}
				auto const obj = cyng::make_object(std::move(vec));
				cyng::io::serialize_json(fso, obj);
				cyng::io::serialize_json(std::cout, obj);
				std::cout << std::endl;
			}
			else
			{
				std::cerr
					<< "** error: unable to open file "
					<< config_.json_path_
					<< std::endl;
			}
		}

		boost::uuids::uuid controller_base::get_random_tag() const {
			return uidgen_();
		}

		int controller_base::run() {

			//
			//	to calculate uptime
			//
			auto const now = std::chrono::system_clock::now();

			//
			//	controller loop
			//
			try {

				//
				//	controller loop
				//
				bool shutdown{ false };
				while (!shutdown) {

					//
					//	Create an I/O controller with specified size
					//	of the thread pool.
					//
					cyng::controller ctl(config_.pool_size_);

					//
					//	read configuration file
					//	start application
					//
					auto const cfg = read_config_section(config_.json_path_, config_.config_index_);

					//std::cout << cfg << std::endl;
					BOOST_ASSERT_MSG(cfg, "no configiration data");
					BOOST_ASSERT_MSG(cyng::is_of_type<cyng::TC_PARAM_MAP>(cfg), "wrong configiration data type");
					shutdown = !cfg;
					if (shutdown) {
						std::cerr
							<< "use option -D to generate a configuration file"
							<< std::endl;
						return EXIT_FAILURE;
					}

					//
					//	setup logging
					//
					cyng::logger logger(ctl.create_channel<cyng::log>());

					//
					//	log level
					//
					logger.set_level(config_.get_log_level());

					//
					//	set appender
					//
 					if (config_.log_console_) {
						//	enable console logging
						logger.start_console_logger();
					}
					if (config_.log_file_) {
						logger.start_file_logger(config_.log_file_path_, config_.log_file_size_);
					}
					
#if defined(BOOST_OS_LINUX_AVAILABLE)
					if (config_.log_syslog_) {
						logger.start_syslog(config_.node_, !config_.log_console_);
					}
#endif

					// Capture SIGINT and SIGTERM to perform a clean shutdown
					boost::asio::signal_set signals(ctl.get_ctx().get_executor().context(), SIGINT, SIGTERM);
					signals.async_wait(
						[&](boost::system::error_code const&, int sig)
						{
							// Stop the `io_context`. This will cause `run()`
							// to return immediately, eventually destroying the
							// `io_context` and all of the sockets in it.
							shutdown = true;

							const auto uptime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - now);

							CYNG_LOG_INFO(logger, "shutdown " << config_.node_ << " - uptime: " << uptime);
							logger.stop();

							//
							//	stop all running tasks
							//	and stops the IO context.
							//
							//ctl.shutdown();
							ctl.get_registry().shutdown();
							std::this_thread::sleep_for(std::chrono::seconds(1));
							std::this_thread::sleep_for(std::chrono::seconds(1));
							std::this_thread::sleep_for(std::chrono::seconds(1));
						});

					CYNG_LOG_INFO(logger, "startup " << config_.node_);

					//
					//	startup application
					//
					run(ctl, logger, cfg);

					//
					//	wait for pending requests
					//
					ctl.stop();
					std::this_thread::sleep_for(std::chrono::seconds(1));
					std::this_thread::sleep_for(std::chrono::seconds(1));
					std::this_thread::sleep_for(std::chrono::seconds(1));
				}

				return EXIT_SUCCESS;
			}
			catch (std::exception& ex)	{
				std::cerr
					<< ex.what()
					<< std::endl;
			}
			return EXIT_FAILURE;

		}

		cyng::object controller_base::read_config_section(std::string const& json_path, std::size_t config_index) {

			auto const obj = cyng::json::parse_file(json_path);
			if (obj) {
				auto vec = cyng::container_cast<cyng::vector_t>(obj);
				if (config_index < vec.size()) {
					return vec.at(config_index);
				}
			}
			return obj;
		}

		void controller_base::print_configuration(std::ostream& os) {
			auto const cfg = read_config_section(config_.json_path_, config_.config_index_);
			std::cout << cfg << std::endl;
			BOOST_ASSERT_MSG(cfg, "no configiration data");
			BOOST_ASSERT_MSG(cyng::is_of_type<cyng::TC_PARAM_MAP>(cfg), "wrong configiration data type");

			cyng::io::serialize_json_pretty(os, cfg);

		}

	}
}

