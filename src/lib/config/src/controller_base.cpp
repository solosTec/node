/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/controller_base.h>

#include <smf.h>

#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/log/log.h>
#include <cyng/log/record.h>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/factory.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/parse/json.h>
#include <cyng/parse/string.h>
#include <cyng/sys/locale.h>
#include <cyng/task/controller.h>
#include <cyng/task/stash.h>

#include <fstream>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace smf {
    namespace config {

        controller_base::controller_base(startup const &config)
            : config_(config)
            , uidgen_() {}

        bool controller_base::run_options(boost::program_options::variables_map &vars) {

            if (vars["default"].as<bool>()) {
                //	write default configuration
                write_config(create_default_config(
                    std::chrono::system_clock::now(), std::filesystem::temp_directory_path(), std::filesystem::current_path()));
                return true;
            }
            if (vars["show"].as<bool>()) {
                //	show configuration
                print_configuration(std::cout);
                return true;
            }
            if (vars["locale"].as<bool>()) {
                //	print system locale
                print_locale(std::cout);
                return true;
            }
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
            if (vars["service.enabled"].as<bool>()) {
                //	run as service
                auto const srv_name = vars["service.name"].as<std::string>();
                ::OutputDebugString(srv_name.c_str());
                return run_as_service<controller_base>(this, srv_name);
            }
#endif

            return false;
        }

        void controller_base::write_config(cyng::vector_t &&vec) {

            // std::cout << cyng::to_string(vec) << std::endl;

            std::fstream fso(config_.json_path_, std::ios::trunc | std::ios::out);
            if (fso.is_open()) {

                std::cout << "write to file " << config_.json_path_ << std::endl;

                //
                //	get default values
                //
                if (vec.empty()) {
                    std::cerr << "** warning: configuration is empty" << std::endl;
                }
                auto const obj = cyng::make_object(std::move(vec));
                cyng::io::serialize_json(fso, obj);
                cyng::io::serialize_json(std::cout, obj);
                std::cout << std::endl;
            } else {
                std::cerr << "** error: unable to open file " << config_.json_path_ << std::endl;
            }
        }

        boost::uuids::uuid controller_base::get_random_tag() const { return uidgen_(); }

        boost::uuids::uuid controller_base::read_tag(cyng::object obj) const {
            auto const rnd = get_random_tag();
            auto const str = cyng::value_cast(obj, boost::uuids::to_string(rnd));
            return cyng::to_uuid(str, rnd);
        }

        int controller_base::run() {

            //
            //	to calculate uptime
            //
            auto const now = std::chrono::system_clock::now();

#if defined(BOOST_OS_WINDOWS_AVAILABLE)
            {
                std::stringstream ss;
                ss << "[" << config_.node_ << "] entry to the main programme";
                auto const msg = ss.str();
                ::OutputDebugString(msg.c_str());
            }
#endif
            //
            //	controller loop
            //
            try {

                //
                //	controller loop
                //
                bool shutdown{false};
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

                    // std::cout << cfg << std::endl;
                    BOOST_ASSERT_MSG(cfg, "no configuration data");
                    BOOST_ASSERT_MSG(cyng::is_of_type<cyng::TC_PARAM_MAP>(cfg), "wrong configiration data type");
                    shutdown = !cfg;
                    if (shutdown) {
                        std::cerr << "use option -D to generate a configuration file" << std::endl;
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
                        {
                            std::stringstream ss;
                            ss << "[" << config_.node_ << "] - use option -D to generate a configuration file";
                            auto const msg = ss.str();
                            ::OutputDebugString(msg.c_str());
                        }
#endif
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
                    if (config_.log_file_ && !config_.log_file_path_.empty()) {
                        logger.start_file_logger(config_.log_file_path_, config_.log_file_size_);
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
                        {
                            std::stringstream ss;
                            ss << "[" << config_.node_ << "] - logs to " << config_.log_file_path_;
                            auto const msg = ss.str();
                            ::OutputDebugString(msg.c_str());
                        }
#endif
                    }

#if defined(BOOST_OS_LINUX_AVAILABLE)
                    if (config_.log_syslog_) {
                        logger.start_syslog(config_.node_, !config_.log_console_);
                    }
#endif

                    cyng::stash channels(ctl.get_ctx());

                    // Capture SIGINT and SIGTERM to perform a clean shutdown
                    boost::asio::signal_set signals(ctl.get_ctx().get_executor().context(), SIGINT, SIGTERM);
                    signals.async_wait([&, this](boost::system::error_code const &, int sig) {
                        // Stop the `io_context`. This will cause `run()`
                        // to return immediately, eventually destroying the
                        // `io_context` and all of the sockets in it.
                        shutdown = true;

                        const auto uptime =
                            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - now);

                        CYNG_LOG_INFO(logger, "shutdown " << config_.node_ << " - uptime: " << uptime);
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
                        { 
                            std::stringstream ss;
                            ss << "shutdown " << config_.node_ << " - uptime: " << uptime;
                            auto const msg = ss.str();
                            ::OutputDebugString(msg.c_str()); 
                        }
#endif

                        //	stop all running tasks
                        this->shutdown(ctl.get_registry(), channels, logger);
                        logger.stop();
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        ctl.get_registry().shutdown();
                        ctl.shutdown();
                    });

                    CYNG_LOG_INFO(logger, "startup " << config_.node_);
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
                    {
                        std::stringstream ss;
                        ss << "[" << config_.node_ << "] startup with: " << config_.config_file_ << "/" << config_.json_path_;
                        auto const msg = ss.str();
                        ::OutputDebugString(msg.c_str());
                    }
#endif

                    //
                    //	startup application
                    //
                    run(ctl, channels, logger, cfg, config_.node_);

                    //
                    //	wait for pending requests
                    //
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    ctl.stop();
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }

                return EXIT_SUCCESS;
            } catch (std::exception &ex) {
                std::cerr << ex.what() << std::endl;
            }
            return EXIT_FAILURE;
        }

        cyng::object controller_base::read_config_section(std::string const &json_path, std::size_t config_index) {

            auto vec = cyng::container_cast<cyng::vector_t>(cyng::json::parse_file(json_path));
            return (config_index < vec.size()) ? vec.at(config_index) : cyng::make_object();
        }

        void controller_base::print_configuration(std::ostream &os) {
            auto const cfg = read_config_section(config_.json_path_, config_.config_index_);
            std::cout << cfg << std::endl;
            BOOST_ASSERT_MSG(cfg, "no configiration data");
            BOOST_ASSERT_MSG(cyng::is_of_type<cyng::TC_PARAM_MAP>(cfg), "wrong configiration data type");

            cyng::io::serialize_json_pretty(os, cfg);
        }

        void controller_base::print_locale(std::ostream &os) {
            try {
                auto const loc = cyng::sys::get_system_locale();
                os << "name    : " << loc.at(cyng::sys::info::NAME) << std::endl;
                os << "country : " << loc.at(cyng::sys::info::COUNTRY) << std::endl;
                os << "language: " << loc.at(cyng::sys::info::LANGUAGE) << std::endl;
                os << "encoding: " << loc.at(cyng::sys::info::ENCODING) << std::endl;

            } catch (std::exception const &ex) {
                os << "***error: " << ex.what() << std::endl;
            }
        }

#if BOOST_OS_WINDOWS
        /**
         * Implementation of the control handler.
         * Forward events from service controller to service.
         */
        void controller_base::control_handler(DWORD sig) {
            //	forward signal to shutdown manager
            // cyng::forward_signal(sig);
        }
#endif

        void stop_tasks(cyng::logger logger, cyng::registry &reg, std::string name) {

            reg.lookup(name, [=](std::vector<cyng::channel_ptr> channels) mutable {
                CYNG_LOG_INFO(logger, "stop " << channels.size() << " task(s) [" << name << "]");
                for (auto channel : channels) {
                    channel->stop();
                }
            });
        }

    } // namespace config
} // namespace smf
