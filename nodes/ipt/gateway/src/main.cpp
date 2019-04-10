/*
 * Copyright Sylko Olzscher 2016-2017
 * 
 * Use, modification, and distribution is subject to the Boost Software
 * License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include "../../../print_build_info.h"
#include "../../../print_version_info.h"
#include "../../../set_start_options.h"
#include "../../../show_ip_address.h"
#include "../../../show_fs_drives.h"
#if BOOST_OS_WINDOWS
#include <boost/asio.hpp>
#endif
#include "controller.h"
#include <iostream>
#include <fstream>

//	test
#include <smf/mbus/header.h>
#include <smf/sml/srv_id_io.h>
#include <stdlib.h>

/**
 * main entry point.
 * To run as windows service additional preparations
 * required.
 */
int main(int argc, char **argv) 
{
// 	{
// 		std::cerr << "test (X1)" << std::endl;
// 		auto inp = cyng::make_buffer({ 0x13, 0x09, 0x00, 0x16, 0xe6, 0x1e, 0x3c, 0x07, 0x3a, 0x00, 0x20, 0x65, 0x3a, 0xc4, 0xef, 0xf7, 0x37, 0x13, 0xd0, 0x9a, 0x92, 0xa7, 0xb5, 0xd9, 0x83, 0xb8, 0x0c, 0x67, 0xac, 0x3b, 0x33, 0x67, 0xc6, 0x9d, 0x3e, 0xe7, 0x56, 0x2b, 0x96, 0x21, 0x26, 0x7c, 0xe2, 0xc9 });
// 		std::cerr << "test (X2)" << std::endl;
// 		auto vec = inp;
// 		std::cerr << "test (X3)" << std::endl;
// 	}
// 	{
// 		std::cerr << "test (A1)" << std::endl;
// 		auto inp = cyng::make_buffer({ 0x13, 0x09, 0x00, 0x16, 0xe6, 0x1e, 0x3c, 0x07, 0x3a, 0x00, 0x20, 0x65, 0x3a, 0xc4, 0xef, 0xf7, 0x37, 0x13, 0xd0, 0x9a, 0x92, 0xa7, 0xb5, 0xd9, 0x83, 0xb8, 0x0c, 0x67, 0xac, 0x3b, 0x33, 0x67, 0xc6, 0x9d, 0x3e, 0xe7, 0x56, 0x2b, 0x96, 0x21, 0x26, 0x7c, 0xe2, 0xc9 });
// 		std::cerr << "test (A2)" << std::endl;
// 		node::header_long hl;
// 		std::cerr << "test (A3)" << std::endl;
// 		node::reset_header_long(hl, 1, inp);
// 		std::cerr << "mode: " << +hl.header().get_mode() << std::endl;
// 		
// 		std::pair<node::header_long, bool> r = node::make_header_long(1, inp);
// 		std::cerr << "mode: " << +r.first.header().get_mode() << std::endl;
// 	}
// 	{
// 		std::cerr << "test (B1)" << std::endl;
// 		auto inp = cyng::make_buffer({ 0xbf, 0x00, 0x23, 0x00, 0x20, 0x05, 0x5d, 0xa8, 0x1f, 0x31, 0xb4, 0xae, 0x36, 0xbc, 0xd3, 0x54, 0xf2, 0xf4, 0x1c, 0x87, 0x1b, 0xd4, 0x32, 0x9c, 0x14, 0x83, 0xa4, 0x0b, 0x6f, 0x01, 0xd7, 0xf5, 0xde, 0x13, 0xf5, 0x4d, 0xfe, 0x08 });
// 		std::pair<node::header_long, bool> r = node::make_header_long(1, inp);
// 		std::cerr << "mode  : " << +r.first.header().get_mode() << std::endl;		
// 		std::cerr << "server: " << node::sml::from_server_id(r.first.get_srv_id()) << std::endl;
// 	}
// 	{
// 		std::cerr << "test (C1)" << std::endl;
// 		auto inp = cyng::make_buffer({ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x23, 0x00, 0x20, 0x05, 0x5d, 0xa8, 0x1f, 0x31, 0xb4, 0xae, 0x36, 0xbc, 0xd3, 0x54, 0xf2, 0xf4, 0x1c, 0x87, 0x1b, 0xd4, 0x32, 0x9c, 0x14, 0x83, 0xa4, 0x0b, 0x6f, 0x01, 0xd7, 0xf5, 0xde, 0x13, 0xf5, 0x4d, 0xfe, 0x08 });
// 		std::pair<node::header_long, bool> r = node::make_header_long(1, inp);
// 		std::cerr << "mode: " << +r.first.header().get_mode() << std::endl;
// 		std::cerr << "server: " << node::sml::from_server_id(r.first.get_srv_id()) << std::endl;
// 	}
// 	
// 	return 0;
	
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
		("config,C", boost::program_options::value<std::string>(&config_file)->default_value(node::get_cfg_name("gateway")), "specify the configuration file")
		//("init,I", boost::program_options::bool_switch()->default_value(false), "initialize database and exit")
		("default,D", boost::program_options::bool_switch()->default_value(false), "generate a default configuration and exit")
		("ip,N", boost::program_options::bool_switch()->default_value(false), "show local IP address and exit")
		("fs,F", boost::program_options::bool_switch()->default_value(false), "show available drives")
		("show", boost::program_options::bool_switch()->default_value(false), "show configuration")
		("console", boost::program_options::bool_switch()->default_value(false), "log (only) to console")

		;
		
	//	path to JSON configuration file
	std::string json_path;
	unsigned int pool_size = 1;
#if BOOST_OS_LINUX
	struct rlimit rl;
	int rc = ::getrlimit(RLIMIT_NOFILE, &rl);
	BOOST_ASSERT_MSG(rc == 0, "getrlimit() failed");
#endif

	//
	//	data node options
	//
	boost::program_options::options_description node_options("gateway");
	node::set_start_options(node_options
		, "gateway"
		, json_path
		, pool_size
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
            return node::print_version_info(std::cout, "ipt::gateway");
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

		if (vm["fs"].as< bool >())
		{
			//	show available drives
			return node::show_fs_drives(std::cout);
		}

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
		node::controller ctrl(pool_size, json_path);

		if (vm["default"].as< bool >())
		{
			//	write default configuration
 			return ctrl.create_config();
		}

		//if (vm["init"].as< bool >())
		//{
		//	//	initialize database
 	//		return ctrl.init_db();
		//}

		if (vm["show"].as< bool >())
		{
			//	show configuration
// 			return ctrl.show_config();
		}

#if BOOST_OS_WINDOWS
		if (vm["service.enabled"].as< bool >())
		{
			::OutputDebugString("start Setup node");

			//	run as service 
			const std::string srv_name = vm["service.name"].as< std::string >();
 			return ctrl.run_as_service(std::move(ctrl), srv_name);
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
