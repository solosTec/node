/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/config.h>

#include <iostream>
#include <boost/program_options.hpp>
#include <boost/predef.h>

 /**
  * main entry point.
  * To run as windows service additional preparations
  * required.
  */
int main(int argc, char** argv) {

	//	will contain the path to an optional configuration file (.cfg)
	smf::config::startup config("LoRa");

	//
	//	generic options
	//
	boost::program_options::options_description generic("Generic options");
	generic.add_options()

		("help,h", "print usage message")
		("version,v", "print version string")
		("build,b", "last built timestamp and platform")
		("config,C", boost::program_options::value<std::string>(&config.config_file_)->default_value(config.cfg_default_), "specify the configuration file")
		("log-level,L", boost::program_options::value< std::string >(&config.log_level_str_)->default_value(config.log_level_str_), "log levels are T[RACE], D[EBUG], I[NFO], W[ARNING], E[RROR] and F[ATAL]")

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

		if (vm.count("help"))
		{
			std::cout
				<< cmdline_options
				<< std::endl
				;
			return EXIT_SUCCESS;
		}

		if (smf::config::complete_generic_options(config, node_options, vm))	return EXIT_SUCCESS;

		//
		//	RLIMIT_NOFILE (linux only)
		//
		smf::config::set_resource_limit(config);

	}
	catch (std::bad_cast const& e) {
		std::cerr
			<< "*** FATAL: "
			<< e.what()
			<< std::endl
			;
	}
	return EXIT_FAILURE;
}
