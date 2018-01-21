/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "print_build_info.h"
#include <NODE_project_info.h>
#include <CYNG_project_info.h>
#include <boost/config.hpp>
#include <boost/version.hpp>
#include <boost/asio/version.hpp>
#include <boost/predef.h>

namespace node 
{
	int print_build_info(std::ostream& os)
	{
		os
		<< "configured at: "
		<< NODE_BUILD_DATE
		<< " UTC"
		<< std::endl
		<< "last built at: "
		<< __DATE__
		<< " "
		<< __TIME__
		<< std::endl
		<< "Platform     : "
		<< NODE_PLATFORM
		<< std::endl
		<< "Compiler     : "
		<< BOOST_COMPILER
		<< std::endl
		<< "StdLib       : "
		<< BOOST_STDLIB
		<< std::endl
		<< "BOOSTLib     : v"
		<< BOOST_LIB_VERSION
		<< " ("
		<< BOOST_VERSION
		<< ")"
		<< std::endl
		<< "Boost.Asio   : v"
		<< (BOOST_ASIO_VERSION / 100000)
		<< '.'
		<< (BOOST_ASIO_VERSION / 100 % 1000)
		<< '.'
		<< (BOOST_ASIO_VERSION % 100)
		<< std::endl
		<< "CyngLib      : v"
		<< CYNG_VERSION
		<< " ("
		<< CYNG_BUILD_DATE
		<< ")"
		<< std::endl
		
// BOOST_ASIO_VERSION % 100 is the sub-minor version
// BOOST_ASIO_VERSION / 100 % 1000 is the minor version
// BOOST_ASIO_VERSION / 100000 is the major version

// 			<< "OpenSSL      : v"
// 			<< NODDY_SSL_VERSION
// 			<< std::endl
		<< "build type   : "
#if BOOST_OS_WINDOWS
#ifdef _DEBUG
		<< "Debug"
#else
		<< "Release"
#endif
#else
		<< NODE_BUILD_TYPE
#endif
		<< std::endl
// 			<< "shared libs  : "
// 			<< NODDY_SHARED_LIBS
// 			<< std::endl
// 		<< "CPU count    : "
// 		<< std::thread::hardware_concurrency()
// 		<< std::endl
		;
        return EXIT_SUCCESS;
	}
}

