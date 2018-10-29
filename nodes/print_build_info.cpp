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
#include <ctime>
#include <chrono>

//
//  Because of the macros __DATE__ and __TIME__ the binary
//  depends on the time it was build. We have to tell GCC that is no
//  problem so far.
//
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdate-time"
#endif

namespace node 
{
	int print_build_info(std::ostream& os)
	{
		os
		<< "configured at : "
		<< NODE_BUILD_DATE
		<< " UTC"
		<< std::endl

		<< "last built at : "
		<< __DATE__
		<< " "
		<< __TIME__
		<< std::endl

		<< "Platform      : "
		<< NODE_PLATFORM
		<< std::endl

		<< "Compiler      : "
		<< BOOST_COMPILER
		<< std::endl

		<< "StdLib        : "
		<< BOOST_STDLIB
		<< std::endl

		<< "BOOSTLib      : v"
		<< (BOOST_VERSION / 100000)		//	major version
		<< '.'
		<< (BOOST_VERSION / 100 % 1000)	//	minor version
		<< '.'
		<< (BOOST_VERSION % 100)	//	patch level
		<< " ("
        << NODE_BOOST_VERSION
		<< ")"
		<< std::endl

		<< "Boost.Asio    : v"
		<< (BOOST_ASIO_VERSION / 100000)
		<< '.'
		<< (BOOST_ASIO_VERSION / 100 % 1000)
		<< '.'
		<< (BOOST_ASIO_VERSION % 100)
		<< std::endl

		<< "CyngLib       : v"
		<< CYNG_VERSION
		<< " ("
		<< CYNG_BUILD_DATE
		<< ")"
		<< std::endl

#ifdef NODE_SSL_INSTALLED
		<< "SSL/TSL       : v"
        << NODE_SSL_VERSION
#else
		<< "SSL/TSL       : not supported"
#endif
        << std::endl

		<< "build type    : "
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

        << "address model : "
        << NODE_ADDRESS_MODEL
        << " bit"
        << std::endl
			

        //<< "std::time_t   : "
        //<< sizeof(std::time_t)
        //<< " bytes"
        //<< std::endl

        //<< "max. timestamp: "
        //<< cyng::to_str(std::chrono::system_clock::time_point::max())
        //<< std::endl

        ;
		

        return EXIT_SUCCESS;
	}
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
