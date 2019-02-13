/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SERIAL_STOPBITS_H
#define NODE_SERIAL_BAUDRATE_H


#include <string>
#include <boost/asio/serial_port_base.hpp>

namespace node
{
	namespace serial	
	{
		/**
		 *	@return stop_bits enum with the specified name.
		 *	If no name is matching function returns boost::asio::serial_port_base::stop_bits::one
		 */
		boost::asio::serial_port_base::stop_bits to_stopbits(std::string);
	}
}	//	node


#endif	//	NODE_SERIAL_STOPBITS_H
