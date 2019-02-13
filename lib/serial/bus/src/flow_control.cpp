/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include <smf/serial/flow_control.h>
#include <boost/algorithm/string.hpp>

namespace node
{
	namespace serial
	{
		boost::asio::serial_port_base::flow_control to_flow_control(std::string s)
		{
			if (boost::algorithm::equals("software", s)) {
				return boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::software);
			}
			else if (boost::algorithm::equals("hardware", s)) {
				return boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::hardware);
			}
			return boost::asio::serial_port_base::flow_control();
		}
	}
}
