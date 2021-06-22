/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf.h>
#include <smf/config.h>

#include <cyng/io/ostream.h>
#include <cyng/log/conv.h>
#include <cyng/log/logger.h>
#include <cyng/sys/host.h>
#include <cyng/sys/mac.h>
#include <cyng/sys/net.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include <boost/assert.hpp>
#include <boost/predef.h>

namespace smf {
    namespace config {

        std::string derive_cfg_file_name(std::string node) {
            BOOST_ASSERT(!node.empty());

#if defined(BOOST_OS_LINUX_AVAILABLE)
#if defined(__ARMEL__)
            return "/usr/local/etc/" + node + "_" + SMF_VERSION_NAME + ".cfg";
#else
            return "/opt/smf/" + node + "_" + SMF_VERSION_NAME + ".cfg";
#endif
#else
            return node + "_" + SMF_VERSION_NAME + ".cfg";
#endif
        }

        std::string derive_json_cfg_file_name(std::string node) {
#if defined(BOOST_OS_LINUX_AVAILABLE)
#if defined(__ARMEL__)
            return "/usr/local/etc/smf/" + node + "_" + SMF_VERSION_NAME + ".json";
#else
            return "/usr/local/etc/" + node + "_" + SMF_VERSION_NAME + ".json";
#endif
#else
            return node + "_" + SMF_VERSION_NAME + ".json";
#endif
        }

#if defined(BOOST_OS_WINDOWS_AVAILABLE)
        std::string derive_service_name(std::string node) {
            BOOST_ASSERT(!node.empty());
            if (!node.empty()) {
                node.at(0) = std::toupper(node.at(0));
            }
#ifdef _DEBUG
            return std::string("smf") + node + "D" + SMF_VERSION_NAME;
#else
            return std::string("smf") + node;
#endif
        }
#endif

        bool get_resource_limit(startup &st) {
#if defined(BOOST_OS_LINUX_AVAILABLE)
            int rc = ::getrlimit(RLIMIT_NOFILE, &st.rl_);
            BOOST_ASSERT_MSG(rc == 0, "getrlimit() failed");
            return rc == 0;
#else
            return true;
#endif
        }
        bool set_resource_limit(startup const &st) {
#if defined(BOOST_OS_LINUX_AVAILABLE)
            auto rc = ::setrlimit(RLIMIT_NOFILE, &st.rl_);
            BOOST_ASSERT_MSG(rc == 0, "setrlimit() failed");
            return rc == 0;
#else
            return true;
#endif
        }

        startup::startup(std::string node)
            : node_(node)
            , cfg_default_(derive_cfg_file_name(node))
            , config_file_()
            , json_path_()
            , config_index_{0}
            , pool_size_ {
            2
        }
#if defined(BOOST_OS_LINUX_AVAILABLE)
        , log_syslog_(cyng::has_sys_log())
#endif
#ifdef _DEBUG
              ,
            log_level_str_(cyng::to_string(cyng::severity::LEVEL_TRACE))
#else
        , log_level_str_(cyng::to_string(cyng::severity::LEVEL_INFO))
#endif
                ,
            log_console_(false), log_file_(false), log_file_path_(), log_file_size_(32UL * 1024UL * 1024UL)
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
                                                                         ,
            log_eventlog_(cyng::has_event_log())
#endif
        {
            BOOST_ASSERT(!node.empty());
#if defined(BOOST_OS_LINUX_AVAILABLE)
            get_resource_limit(*this);
#endif
        }

        cyng::severity startup::get_log_level() const noexcept { return cyng::to_severity(log_level_str_); }

        boost::program_options::options_description get_generic_options(startup &config) {
            boost::program_options::options_description generic("Generic options");
            generic.add_options()

                ("help,h",
                 "print usage message")("version,v", "print version string")("build,b", "last built timestamp and platform")(
                    "net,N", boost::program_options::bool_switch()->default_value(false), "show local network configuration")(
                    "config,C",
                    boost::program_options::value<std::string>(&config.config_file_)->default_value(config.cfg_default_),
                    "specify the configuration file")(
                    "log-level,L",
                    boost::program_options::value<std::string>(&config.log_level_str_)->default_value(config.log_level_str_),
                    "log levels are T[RACE], D[EBUG], I[NFO], W[ARNING], E[RROR] and F[ATAL]")(
                    "default,D",
                    boost::program_options::bool_switch()->default_value(false),
                    "generate a default configuration and exit")(
                    "show,s", boost::program_options::bool_switch()->default_value(false), "show configuration")

                ;

            return generic;
        }

        boost::program_options::options_description get_default_options(startup &start) {

            auto const current_path = std::filesystem::current_path();
            auto const json_path_default = (current_path / (start.node_ + "_" + SMF_VERSION_NAME + ".json")).string();
            //	don't use more than 8 threads
            std::size_t const pool_size = std::min<std::size_t>(std::thread::hardware_concurrency(), 4) * 2ul;

            auto log_file_path = (std::filesystem::temp_directory_path() / start.node_).replace_extension(".log").string();
            auto log_file_size = 32UL * 1024UL * 1024UL;

            boost::program_options::options_description options(start.node_);
            options.add_options()
                ("setup.json,J", boost::program_options::value(&start.json_path_)->default_value(json_path_default),"JSON configuration file")  // --setup.json
                ("setup.config-index", boost::program_options::value(&start.config_index_)->default_value(start.config_index_),"Default configuration index")   // --setup.config-index
                ("setup.pool-size,P", boost::program_options::value(&start.pool_size_)->default_value(pool_size), "Thread pool size")
#if defined(BOOST_OS_LINUX_AVAILABLE)
                ("RLIMIT_NOFILE.soft",
                 boost::program_options::value<rlim_t>()->default_value(start.rl_.rlim_cur),
                 "RLIMIT_NOFILE soft")(
                    "RLIMIT_NOFILE.hard",
                    boost::program_options::value<rlim_t>()->default_value(start.rl_.rlim_max),
                    "RLIMIT_NOFILE hard")(
                    "log.syslog",
                    boost::program_options::bool_switch(&start.log_syslog_)->default_value(cyng::has_sys_log()),
                    "start syslog logger")
#endif
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
                ("service.enabled,S", boost::program_options::value<bool>()->default_value(false), "run as NT service") // -- service.enabled
                ("service.name", boost::program_options::value<std::string>()->default_value(derive_service_name(start.node_)),"NT service name")   // -- service.name
                ("log.eventlog",
                        boost::program_options::bool_switch(&start.log_eventlog_)->default_value(cyng::has_event_log()),
                        "start event logger")
#endif
                        ("log.console",
                         boost::program_options::bool_switch(&start.log_console_)->default_value(true),
                         "start console logger")(
                            "log.file",
                            boost::program_options::bool_switch(&start.log_file_)->default_value(true),
                            "start file logger")(
                            "log.file-name",
                            boost::program_options::value<std::string>(&start.log_file_path_)->default_value(log_file_path),
                            "log file name")(
                            "log.file-size",
                            boost::program_options::value<std::uint64_t>(&start.log_file_size_)->default_value(log_file_size),
                            "log file size (bytes)");

            return options;
        }

        bool complete_generic_options(
            startup const &start,
            boost::program_options::options_description &node_options,
            boost::program_options::variables_map &vars) {

            if (vars.count("version")) {
                print_version_info(std::cout, start.node_);
                return true;
            }
            if (vars.count("build")) {
                print_build_info(std::cout);
                return true;
            }

            if (vars["net"].as<bool>()) {
                //	print network configuration
                print_net_config(std::cout);
                return true;
            }

            auto const cfg = vars["config"].as<std::string>();
            read_cfg_file(cfg, node_options, vars);

            return false;
        }

        void read_cfg_file(
            std::string file_name,
            boost::program_options::options_description &node_options,
            boost::program_options::variables_map &vars) {

            std::ifstream ifs(file_name);
            if (ifs.is_open()) {
                //
                //	options available from config file
                //	read parameters from config file
                boost::program_options::store(boost::program_options::parse_config_file(ifs, node_options), vars);

                //	update local values
                boost::program_options::notify(vars);
            }
        }

        void print_version_info(std::ostream &os, std::string const &name) {
            os << name << " SMF v" << SMF_VERSION_TAG << std::endl
               << "Copyright (C) 2012-" << cyng::sys::get_build_year() << " S. Olzscher (solos::Tec). All rights reserved."
               << std::endl;
        }

        void print_build_info(std::ostream &os) {

            os << "build info     : " << std::endl
               << "processor      : " << SMF_SYSTEM_PROCESSOR << std::endl
               << "               : " << cyng::sys::get_host_processor_description() << std::endl

#if defined(__ARMEL__)
               << "ARMEL          : yes" << std::endl
#endif
               << "system         : " << SMF_SYSTEM_NAME << " v" << SMF_SYSTEM_VERSION << std::endl

               << "based on cyng  : " << cyng::sys::get_build_time() << " UTC" << std::endl

               << "Boost library  : v" << (cyng::sys::get_boost_version() / 100000) //	major version
               << '.' << (cyng::sys::get_boost_version() / 100 % 1000)              //	minor version
               << '.' << (cyng::sys::get_boost_version() % 100)                     //	patch level
               << " (" << cyng::sys::get_boost_version() << ")" << std::endl

               << "Asio library   : v" << (cyng::sys::get_asio_version() / 100000) //	major version
               << '.' << (cyng::sys::get_asio_version() / 100 % 1000)              //	minor version
               << '.' << (cyng::sys::get_asio_version() % 100)                     //	patch level
               << " (" << cyng::sys::get_asio_version() << ")" << std::endl

               << "cpp version    : " << cyng::sys::get_cpp_version() << std::endl

               << "cross compiled : " << (cyng::sys::is_crosscompiled() ? "yes" : "no") << std::endl

               << "logical cores  : " << cyng::sys::get_host_number_of_logical_cores() << std::endl

               << "physical cores : " << cyng::sys::get_host_number_of_physical_cores() << std::endl

               << "virt. memory   : " << cyng::sys::get_host_total_virtual_memory() << " MB" << std::endl

               << "phys. memory   : " << cyng::sys::get_host_total_physical_memory() << " MB" << std::endl

                ;
        }

        void print_net_config(std::ostream &os) {

            auto const macs = cyng::sys::get_mac48_adresses();

            // os
            //	<< "host name: "
            //	<< host
            //	<< std::endl
            //<< "effective OS: "
            //<< cyng::sys::get_os_name()
            //<< std::endl
            //;

            if (!macs.empty()) {
                std::cout << macs.size() << " physical address(es)" << std::endl;
                for (auto const &m : macs) {
                    using cyng::operator<<;
                    std::cout << m;
                    if (cyng::is_private(m)) {
                        std::cout << "\t(private)";
                    }
                    std::cout << std::endl;
                }
            }

            auto const nics = cyng::sys::get_nic_names();
            if (!nics.empty()) {
                std::cout << std::endl;
                std::cout << nics.size() << " network interface controller(s)" << std::endl;
                for (auto const &nic : nics) {
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
                    std::cout << nic << std::endl;
#else
                    std::cout << nic << " - " << cyng::sys::get_address_IPv6(nic) << std::endl;
#endif
                }
            }

            try {
                std::string const host = boost::asio::ip::host_name();
                std::cout << std::endl << "address(es) of \"" << host << "\"" << std::endl;
                boost::asio::io_service io_service;
                boost::asio::ip::tcp::resolver resolver(io_service);
                boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(host, "");

                while (it != boost::asio::ip::tcp::resolver::iterator()) {
                    boost::asio::ip::address addr = (it++)->endpoint().address();

                    std::cout << addr.to_string() << std::endl;
                }
            } catch (std::exception const &) {
            }
        }

    } // namespace config
} // namespace smf
