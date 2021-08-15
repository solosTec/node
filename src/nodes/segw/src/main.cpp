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

    //
    //	generic options
    //
    auto generic = smf::config::get_generic_options(config);
    generic.add_options()(
        "init,I", boost::program_options::bool_switch()->default_value(false), "initialize database and exit") //  --init
        ("transfer,T",
         boost::program_options::bool_switch()->default_value(false),
         "transfer JSON configuration into database")                                                                // --transfer
        ("clear", boost::program_options::bool_switch()->default_value(false), "delete configuration from database") //  --clear
        ("list,l", boost::program_options::bool_switch()->default_value(false), "list configuration from database")  //  --list
        ("set-value",
         boost::program_options::value<std::vector<std::string>>()->multitoken()->composing(),
         "set configuration value: \"path\" \"value\" \"type\"") //  --set-value
        ("add-value",
         boost::program_options::value<std::vector<std::string>>()->multitoken()->composing(),
         "add configuration value: \"path\" \"value\" \"type\"") //  --add-value
        ("del-value",
         boost::program_options::value<std::string>() /*->default_value("")*/,
         "remove configuration value: \"path\"") // --del-value
        ("switch-gpio",
         boost::program_options::value<std::vector<std::string>>()->multitoken()->composing(),
         "switch GPIO: \"number\" [on|off]") //  --switch-gpio
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
