/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include <smf/serial/parity.h>
#include <boost/algorithm/string.hpp>

namespace node
{
	namespace serial
	{
		boost::asio::serial_port_base::parity to_parity(std::string s)
		{
			if (boost::algorithm::equals("odd", s)) {
				return boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::odd);
			}
			else if (boost::algorithm::equals("even", s)) {
				return boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::even);
			}
			return boost::asio::serial_port_base::parity();
		}
	}
}
