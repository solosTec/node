/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include <boost/asio.hpp>
#include "show_ports.h"
#include <cyng/sys/port.h>
// #include <cyng/io/serializer.h>

namespace node 
{
	int show_ports(std::ostream& os)
	{

		auto ports = cyng::sys::get_ports();
		for (auto const& port : ports) {
			os
				<< port
				<< std::endl
				;
		}
		return EXIT_SUCCESS;
	}
}

