/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>

#include <boost/predef.h>
#include <boost/program_options.hpp>
#include <iostream>

/**
 * main entry point.
 * To run as windows service additional preparations
 * required.
 */
int main(int argc, char **argv) {

    //	will contain the path to an optional configuration file (.cfg)
    smf::config::startup config("segw");
    std::string table_name;      //  empty
    std::string transfer = "in"; //  in/out
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
    std::string port_name = "COM3";
#else
#if (OECP_VERSION == 1)
    std::string port_name = "/dev/ttyAPP1",
#elif (OECP_VERSION == 2)
    std::string port_name = "/dev/ttymxc1";
#else
    std::string port_name = "/dev/ttyS0";
#endif
#endif

    //
    //	generic options
    //
    auto generic = smf::config::get_generic_options(config);
    generic.add_options()(
        "init,I", boost::program_options::bool_switch()->default_value(false), "initialize database and exit") //  --init
        ("transfer,T",
         // boost::program_options::bool_switch()->default_value(false),
         boost::program_options::value<std::string>()->implicit_value(transfer)->default_value(""),
         "transfer JSON configuration [in|out] database")                                                            // --transfer
        ("clear", boost::program_options::bool_switch()->default_value(false), "delete configuration from database") //  --clear
        ("list,l", boost::program_options::bool_switch()->default_value(false), "list configuration from database")  //  --list
        ("set-value",
         boost::program_options::value<std::vector<std::string>>()->multitoken()->composing(),
         "set configuration value: \"path\" \"value\" \"type\"") //  --set-value
        ("add-value",
         boost::program_options::value<std::vector<std::string>>()->multitoken()->composing(),
         "add configuration value: \"path\" \"value\" \"type\"")                                            // --add-value
        ("del-value", boost::program_options::value<std::string>(), "remove configuration value: \"path\"") // --del-value
        ("switch-gpio",
         boost::program_options::value<std::vector<std::string>>()->multitoken()->composing(),
         "switch GPIO: \"number\" [on|off]")                                                               //  --switch-gpio
        ("nms-default", boost::program_options::bool_switch()->default_value(false), "print NMS defaults") //  --nms-default
        ("nms-mode",
         boost::program_options::value<std::string>(),
         "set NMS mode [production|test|local]") //  --nms-mode=[production|test|local]
        ("alter,A",
         boost::program_options::value<std::string>()->default_value(table_name),
         "drop and re-create table") //	alter DB
        ("tty",
         boost::program_options::value<std::string>()->implicit_value(port_name)->default_value(""),
         "show configuration of specified port") //	tty
        ;

    //
    //	cmdline_options contains all generic and node specific options
    //	from command line
    //
    auto node_options = smf::config::get_default_options(config);
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic).add(node_options);
    boost::program_options::variables_map vm;

    try {
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(cmdline_options).run(), vm);
        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << cmdline_options << std::endl;
            return EXIT_SUCCESS;
        }

        if (smf::config::complete_generic_options(config, node_options, vm))
            return EXIT_SUCCESS;

        //
        //	RLIMIT_NOFILE (linux only)
        //
        smf::config::set_resource_limit(config);

        smf::controller ctl(config);
        if (ctl.run_options(vm))
            return EXIT_SUCCESS;
        return ctl.controller_base::run();

    } catch (std::bad_cast const &e) {
        std::cerr << "*** FATAL: " << e.what() << std::endl;
    }
    return EXIT_FAILURE;
}
