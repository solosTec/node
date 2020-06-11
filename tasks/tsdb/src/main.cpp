/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "../../../nodes/print_build_info.h"
#include "../../print_version_info.h"
#include "../../../nodes/set_start_options.h"
#include "../../../nodes/show_ip_address.h"
#if BOOST_OS_WINDOWS
#include <boost/asio.hpp>
#endif
#include "controller.h"
#include <cyng/compatibility/file_system.hpp>
#include <iostream>

/**
 * main entry point.
 * To run as windows service additional preparations
 * required.
 */
int main(int argc, char **argv) 
{
	
	//	will contain the path to an optional configuration file
	std::string config_file;
	
	//
	//	generic options
	//
	boost::program_options::options_description generic("Generic options");
	generic.add_options()
		
		("help,h", "print usage message")
		("version,v", "print version string")
		("build,b", "last built timestamp and platform")
		("config,C", boost::program_options::value<std::string>(&config_file)->default_value(node::get_cfg_name("tsdb")), "specify the configuration file")
		("default,D", boost::program_options::bool_switch()->default_value(false), "generate a default configuration and exit")
		("ip,N", boost::program_options::bool_switch()->default_value(false), "show local IP address and exit")
		("show,s", boost::program_options::bool_switch()->default_value(false), "show configuration")
		("console", boost::program_options::bool_switch()->default_value(false), "log (only) to console")

		;
		
	//	path to JSON configuration file
	std::string json_path;
	unsigned int config_index = 0u;
	unsigned int pool_size = 4u;

#if BOOST_OS_LINUX
	struct rlimit rl;
	int rc = ::getrlimit(RLIMIT_NOFILE, &rl);
	BOOST_ASSERT_MSG(rc == 0, "getrlimit() failed");
#endif
	
	//
	//	tsdb task options
	//
	boost::program_options::options_description node_options("tsdb");
	node::set_start_options(node_options
		, "tsdb"
		, json_path
		, pool_size
		, config_index
#if BOOST_OS_LINUX
		, rl
#endif
		);
		
	//
	//	all you can grab from the command line
	//
	boost::program_options::options_description cmdline_options;
	cmdline_options.add(generic).add(node_options);
	
	//
	//	read all data
	//
#if BOOST_OS_WINDOWS
	for (auto idx = 0; idx < argc; ++idx)
	{
		::OutputDebugString(argv[idx]);
	}
#endif
	boost::program_options::variables_map vm;
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
	
	if (vm.count("version")) 
	{  
		return node::print_version_info(std::cout, "tsdb");
	}
	
	if (vm.count("build")) 
	{  
		return node::print_build_info(std::cout);
	}

	if (vm["ip"].as< bool >())
	{
		//	show local IP adresses
		return node::show_ip_address(std::cout);
	}
	
	try
	{
		//	read parameters from config file
		const std::string cfg = vm["config"].as< std::string >();
		std::ifstream ifs(cfg);
		if (ifs.is_open())
		{
			//
			//	options available from config file
			//
			boost::program_options::options_description file_options;
			file_options.add(node_options);//.add(authopt);

			//	read parameters from config file
			boost::program_options::store(boost::program_options::parse_config_file(ifs, file_options), vm);

			//	update local values
			boost::program_options::notify(vm);

		}

		//
		//	establish controller
		//
		node::controller ctrl(config_index, pool_size, json_path, "task:tsdb");

		if (vm["default"].as< bool >())
		{
			//	write default configuration
			return ctrl.ctl::create_config();	//	base class method is hidden
		}

		if (vm["show"].as< bool >())
		{
			//	show configuration
			return ctrl.ctl::print_config(std::cout);
		}

#if BOOST_OS_WINDOWS
		if (vm["service.enabled"].as< bool >())
		{
			//	run as service 
			const std::string srv_name = vm["service.name"].as< std::string >();
			::OutputDebugString(srv_name.c_str());
 			return node::run_as_service(std::move(ctrl), srv_name);
		}
#endif

#if BOOST_OS_LINUX
		rc = ::setrlimit(RLIMIT_NOFILE, &rl);
		BOOST_ASSERT_MSG(rc == 0, "setrlimit() failed");
#endif
		BOOST_ASSERT_MSG(pool_size != 0, "empty thread pool");
		return ctrl.run(vm["console"].as< bool >());
	}
	catch (std::bad_cast const& e)
	{
		std::cerr
			<< "*** FATAL: "
			<< e.what()
			<< std::endl
			;
	}
	return EXIT_FAILURE;
}
