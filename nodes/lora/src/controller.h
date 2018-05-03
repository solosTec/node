/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_HTTP_CONTROLLER_H
#define NODE_HTTP_CONTROLLER_H

#include <string>
#include <cstdint>
#include <boost/predef.h>	//	requires Boost 1.55
#if BOOST_OS_WINDOWS
#include <boost/asio.hpp>
#endif

namespace node 
{
	class controller 
	{
	public:
		/**
		 * @param pool_size thread pool size 
		 * @param json_path path to JSON configuration file 
		 */
		controller(unsigned int pool_size, std::string const& json_path);
		
		/**
		 * Start controller loop.
		 * The controller loop controls startup and shutdown of the server.
		 * 
		 * @return EXIT_FAILURE in case of an error, otherwise EXIT_SUCCESS.
		 */
		int run(bool console);
		
		/**
		 * create a configuration file with default values.

		 * @return EXIT_FAILURE in case of an error, otherwise EXIT_SUCCESS.
		 */
		int create_config() const;

		/**
		 * Generate X509 Certificate
		 */
		int generate_x509(std::string const& c
			, std::string const& l
			, std::string const& o
			, std::string const& cn);

#if BOOST_OS_WINDOWS
		/**
		* run as windows service
		*/
		static int run_as_service(controller&&, std::string const&);
		virtual void control_handler(DWORD);

#endif

	private:
		const unsigned int pool_size_;
		const std::string json_path_;
	};
}

#endif
