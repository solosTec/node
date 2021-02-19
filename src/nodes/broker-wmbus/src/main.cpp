/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>

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
	smf::config::startup config("broker-wmbus");

	//
	//	generic options
	//
	boost::program_options::options_description generic = smf::config::get_generic_options(config);

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

		smf::controller	ctl(config);
		if (ctl.run_options(vm))	return EXIT_SUCCESS;
				return ctl.controller_base::run();

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