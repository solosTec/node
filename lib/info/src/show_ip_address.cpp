/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include <boost/asio.hpp>
#include "show_ip_address.h"
#include <cyng/sys/mac.h>
#include <cyng/sys/info.h>
#include <cyng/sys/ip.h>
#include <cyng/sys/dns.h>
#include <cyng/io/serializer.h>

namespace node 
{
	int show_ip_address(std::ostream& os)
	{
		std::string const host = boost::asio::ip::host_name();
		os
			<< "host name: "
			<< host
			<< std::endl
			<< "effective OS: "
			<< cyng::sys::get_os_name()
			<< std::endl
			;


		auto addrs = cyng::sys::get_adapters();
		for (auto const& a : addrs) {
			os
				<< (a.is_v4() ? "IPv4: " : "IPv6: ")
				<< a.to_string()
				<< std::endl
				;

		}
		try
		{
			auto macs = cyng::sys::retrieve_mac48();
			if (!macs.empty())
			{
				std::cout << macs.size() << " physical address(es)" << std::endl;
				for (auto const& m : macs) {
					using cyng::io::operator<<;
					std::cout
						<< m
						<< std::endl;
				}
			}

			std::cout
				<< "WAN address: "
				<< cyng::sys::get_WAN_address("8.8.8.8")
				<< std::endl
				;

			auto dns = cyng::sys::get_dns_servers();
			std::cout << dns.size() << " DNS server(s)" << std::endl;
			for (auto const& srv : dns) {
				std::cout
					<< srv
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

