/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SERIAL_FLOW_CONTROL_H
#define NODE_SERIAL_FLOW_CONTROL_H


#include <string>
#include <boost/asio/serial_port_base.hpp>

namespace node
{
	namespace serial	
	{
		/**
		 *	@return parity enum with the specified name.
		 *	If no name is matching function returns boost::asio::serial_port_base::flow_control::none
		 */
		boost::asio::serial_port_base::flow_control to_flow_control(std::string);
		std::string to_str(boost::asio::serial_port_base::flow_control);
	}
}	//	node


#endif	//	NODE_SERIAL_FLOW_CONTROL_H
