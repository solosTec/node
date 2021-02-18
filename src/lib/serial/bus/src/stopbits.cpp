/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/serial/stopbits.h>
#include <boost/algorithm/string.hpp>

namespace smf {

	namespace serial {

		boost::asio::serial_port_base::stop_bits to_stopbits(std::string s)
		{
			if (boost::algorithm::equals("onepointfive", s)) {
				return boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::onepointfive);
			}
			else if (boost::algorithm::equals("two", s)) {
				return boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::two);
			}
			//	one
			return boost::asio::serial_port_base::stop_bits();
		}

		std::string to_str(boost::asio::serial_port_base::stop_bits s)
		{
			//	(t != one && t != onepointfive && t != two
			switch (s.value()) {
			case boost::asio::serial_port_base::stop_bits::onepointfive: return "onepointfive";
			case boost::asio::serial_port_base::stop_bits::two: return "two";
			default:
				break;
			}
			return "one";
		}

	}
}
