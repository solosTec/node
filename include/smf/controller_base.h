/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CONFIG_CONTROLLER_BASE_H
#define SMF_CONFIG_CONTROLLER_BASE_H

#include <smf/config.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/tag.hpp>

#include <filesystem>

#include <boost/predef.h> //	requires Boost 1.55
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>

#if BOOST_OS_WINDOWS
#include <cyng/scm/service.hpp>
#endif

namespace cyng {
    class controller;
    class stash;
} // namespace cyng

namespace smf {
    namespace config {

        class controller_base {
          public:
            controller_base(startup const &);

            /**
             * Prepare a default environment and call the derived run() method
             */
            virtual int run();

            /**
             * Evaluate the transfer parameters
             */
            virtual bool run_options(boost::program_options::variables_map &);

#if BOOST_OS_WINDOWS
            /**
             * Implementation of the control handler.
             * Forward events from service controller to service.
             */
            void control_handler(DWORD);
#endif

          protected:
            virtual cyng::vector_t create_default_config(
                std::chrono::system_clock::time_point &&,
                std::filesystem::path &&tmp,
                std::filesystem::path &&cwd) = 0;
            virtual void print_configuration(std::ostream &);

            virtual void
            run(cyng::controller &, cyng::stash &, cyng::logger, cyng::object const &cfg, std::string const &node_name) = 0;
            virtual void shutdown(cyng::registry &, cyng::stash &, cyng::logger) = 0;

            void write_config(cyng::vector_t &&);
            void print_locale(std::ostream &os);

            /**
             * generate a random UUID without requiring system entropy
             */
            [[nodiscard]] boost::uuids::uuid get_random_tag() const;

            /**
             * Convert an object of type string into an UUID
             */
            [[nodiscard]] boost::uuids::uuid read_tag(cyng::object) const;

            [[nodiscard]] cyng::object read_config_section(std::string const &json_path, std::size_t config_index);

          private:
#if BOOST_OS_WINDOWS
            /**
             * run as windows service
             */
            template <typename CTL> int run_as_service(CTL *ctrl, std::string const &srv_name) {
                //
                //	define service type
                //
                using service_type = service<CTL>;

                //
                //	messages
                //
                static const std::string msg_01 = "startup service [" + srv_name + "]";
                static const std::string msg_02 = "An instance of the [" + srv_name + "] service is already running";
                static const std::string msg_03 =
                    "***Error 1063: The [" + srv_name + "] service process could not connect to the service controller";
                static const std::string msg_04 =
                    "The [" + srv_name + "] service is configured to run in does not implement the service";

                //
                //	create service
                //
                ::OutputDebugString(msg_01.c_str());
                service_type srv(ctrl, srv_name);

                //
                //	starts dispatcher and calls service main() function
                //
                const DWORD r = srv.run();
                switch (r) {
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
#ifdef _DEBUG
                    std::cerr << msg_03 << std::endl;
#endif
                    break;
                case ERROR_SERVICE_NOT_IN_EXE:
                    //	The executable program that this service is configured to run in does not implement the service.
                    ::OutputDebugString(msg_04.c_str());
                    break;
                default: {
                    std::stringstream ss;
                    ss << '[' << srv_name << "] service dispatcher stopped: " << r;
                    const std::string msg = ss.str();
                    ::OutputDebugString(msg.c_str());
                } break;
                }

                return EXIT_SUCCESS;
            }

#endif

          protected:
            startup const &config_;

          private:
            mutable boost::uuids::random_generator_mt19937 uidgen_;
            std::unique_ptr<boost::asio::signal_set> signal_;
            bool shutdown_;
        };

        void stop_tasks(cyng::logger, cyng::registry &, std::string);
    } // namespace config
} // namespace smf

#endif
