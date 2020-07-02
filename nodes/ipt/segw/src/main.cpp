/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "../../../print_build_info.h"
#include "../../../print_version_info.h"
#include "../../../set_start_options.h"
#include "../../../show_ip_address.h"
#include "../../../show_fs_drives.h"
#include "../../../show_ports.h"
#if BOOST_OS_WINDOWS
#include <boost/asio.hpp>
#endif
#include "controller.h"
#include <iostream>
#include <fstream>


/**
 * main entry point.
 * To run as windows service additional preparations
 * required.
 */
int main(int argc, char **argv) 
{
	
	//	will contain the path to an optional configuration file
	std::string config_file;
	std::uint32_t profile{ 0 };
	std::string value{ "bool:rs485:enabled:true" };

	//
	//	generic options
	//
	boost::program_options::options_description generic("Generic options");
	generic.add_options()
		
		("help,h", "print usage message")
		("version,v", "print version string")
		("build,b", "last built timestamp and platform")
		("config,C", boost::program_options::value<std::string>(&config_file)->default_value(node::get_cfg_name("segw")), "specify the configuration file")
		("default,D", boost::program_options::bool_switch()->default_value(false), "generate a default configuration and exit")
		("init,I", boost::program_options::bool_switch()->default_value(false), "initialize database and exit")
		("ip,N", boost::program_options::bool_switch()->default_value(false), "show local IP address and exit")
		("fs,F", boost::program_options::bool_switch()->default_value(false), "show available drives")
		("ports,p", boost::program_options::bool_switch()->default_value(false), "show available serial ports")
		("show,s", boost::program_options::bool_switch()->default_value(false), "show JSON configuration")
		("list,l", boost::program_options::bool_switch()->default_value(false), "list configuration from database")
		("console", boost::program_options::bool_switch()->default_value(false), "log (only) to console")
		("transfer,T", boost::program_options::bool_switch()->default_value(false), "transfer JSON configuration into database")
		("clear", boost::program_options::bool_switch()->default_value(false), "delete configuration from database")
		("dump", boost::program_options::value<std::uint32_t>(&profile)->default_value(0)->implicit_value(11), "dump profile data (11 .. 18), 1 == PushOps, 2 == Devices")
		("set-value", boost::program_options::value<std::string>(&value), "set configuration value")
		;

	//	path to JSON configuration file
	std::string json_path;
	unsigned int config_index = 0u;
	unsigned int pool_size = 2u;

#if BOOST_OS_LINUX
	struct rlimit rl;
	int rc = ::getrlimit(RLIMIT_NOFILE, &rl);
	BOOST_ASSERT_MSG(rc == 0, "getrlimit() failed");
#endif

	//
	//	data node options
	//
	boost::program_options::options_description node_options("segw");
	node::set_start_options(node_options
		, "segw"
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
    try
    {
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
            return node::print_version_info(std::cout, "ipt::segw");
        }

        if (vm.count("build"))
        {
            return node::print_build_info(std::cout);
        }

        if (vm["ip"].as< bool >())
        {
            //	show local IP addresses
            return node::show_ip_address(std::cout);
        }

		if (vm["fs"].as< bool >())
		{
			//	show available drives
			return node::show_fs_drives(std::cout);
		}

		if (vm["ports"].as< bool >())
		{
			//	show available ports
			return node::show_ports(std::cout);
		}
		
		//	read parameters from config file
		auto const cfg = vm["config"].as< std::string >();
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
		node::controller ctrl(config_index, pool_size, json_path, "ipt:segw");

		if (vm["default"].as< bool >())
		{
			//	write default configuration
			return ctrl.ctl::create_config();	//	base class method is hidden
		}

		if (vm["init"].as< bool >())
		{
			//	initialize database
 			return ctrl.init_config_db("DB");
		}

		if (vm["show"].as< bool >())
		{
			//	show JSON configuration
 			return ctrl.ctl::print_config(std::cout);
		}

		if (vm["list"].as< bool >())
		{
			//	show database configuration
			return ctrl.list_config(std::cout);
		}

		if (vm["transfer"].as< bool >())
		{
			//	transfer JSON configuration into database
			return ctrl.transfer_config();
		}

		if (vm["clear"].as< bool >())
		{
			//	clear configuration from database
			return ctrl.clear_config();
		}
		

		auto const dump = vm["dump"].as< std::uint32_t >();
		if (dump != 0) {
			//	dump profile data
			return ctrl.dump_profile(dump);
		}

		if (vm.count("set-value"))
		{
			auto const kv = vm["set-value"].as< std::string >();
			return ctrl.set_value(kv);
		}

#if BOOST_OS_WINDOWS
		if (vm["service.enabled"].as< bool >())
		{
			::OutputDebugString("start Setup node");

			//	run as service 
			const std::string srv_name = vm["service.name"].as< std::string >();
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
    catch (boost::program_options::error const& err)   {
        std::cerr
            << "*** FATAL: "
            << err.what()
            << std::endl
            ;
        std::cout << generic << std::endl;
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
