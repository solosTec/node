/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SERIAL_PARITY_H
#define NODE_SERIAL_PARITY_H


#include <string>
#include <boost/asio/serial_port_base.hpp>

namespace node
{
	namespace serial	
	{
		/**
		 *	@return parity enum with the specified name.
		 *	If no name is matching function returns boost::asio::serial_port_base::parity::none
		 */
		boost::asio::serial_port_base::parity to_parity(std::string);
		std::string to_str(boost::asio::serial_port_base::parity);
	}
}	//	node


#endif	//	NODE_SERIAL_PARITY_H
