/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_IP_IO_H
#define NODE_SML_IP_IO_H

 /** @file ip_io.h
  * Conversion of IP adresses and names
  */

#include <cyng/object.h>
#include <boost/asio/ip/address.hpp>

namespace node
{
	namespace sml
	{
		boost::asio::ip::address to_ip_address_v4(cyng::object obj);
		std::string ip_address_to_str(cyng::object obj);

	}	//	sml
}	//	node


#endif	
