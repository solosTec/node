/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include <boost/asio.hpp>
#include "show_ip_address.h"
// #include <NODE_project_info.h>
// #include <boost/config.hpp>
// #include <boost/version.hpp>

namespace node 
{
	int show_ip_address(std::ostream& os)
	{
		const std::string host = boost::asio::ip::host_name();
		os
			<< "host name: "
			<< host
			<< std::endl
			;

		try
		{
			boost::asio::io_service io_service;
			boost::asio::ip::tcp::resolver resolver(io_service);
			boost::asio::ip::tcp::resolver::query query(host, "");
			boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
			boost::asio::ip::tcp::resolver::iterator end; // End marker.
			while (iter != end)
			{
				boost::asio::ip::tcp::endpoint ep = *iter++;
				os
					<< (ep.address().is_v4() ? "IPv4: " : "IPv6: ")
					<< ep
					<< std::endl
					;
			}
			return EXIT_SUCCESS;
		}
		catch (std::exception const& ex)
		{
			os
				<< "***Error: "
				<< ex.what()
				<< std::endl
				;
		}
		return EXIT_FAILURE;
	}
}

