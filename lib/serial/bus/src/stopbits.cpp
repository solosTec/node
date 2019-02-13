/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include <smf/serial/stopbits.h>
#include <boost/algorithm/string.hpp>

namespace node
{
	namespace serial
	{
		boost::asio::serial_port_base::stop_bits to_stopbits(std::string s)
		{
			if (boost::algorithm::equals("onepointfive", s)) {
				return boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::onepointfive);
			}
			else if (boost::algorithm::equals("two", s)) {
				return boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::two);
			}
			return boost::asio::serial_port_base::stop_bits();
		}

	}
}
