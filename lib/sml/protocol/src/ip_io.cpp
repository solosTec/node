/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */


#include <smf/sml/ip_io.h>
#include <cyng/io/swap.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/value_cast.hpp>
#include <cyng/io/serializer.h>
#include <cyng/io/io_buffer.h>
#include <sstream>

namespace node
{
	namespace sml
	{
		boost::asio::ip::address to_ip_address_v4(cyng::object obj)
		{
			const boost::asio::ip::address_v4::uint_type ia = cyng::numeric_cast<boost::asio::ip::address_v4::uint_type>(obj, 0u);
			return boost::asio::ip::make_address_v4(cyng::swap_num(ia));
		}

		std::string ip_address_to_str(cyng::object obj)
		{
			std::stringstream ss;
			if (obj.get_class().tag() == cyng::TC_UINT32) {

				//
				//	convert an u32 to a IPv4 address in dotted format
				//
				ss << to_ip_address_v4(obj);
			}
			else if (obj.get_class().tag() == cyng::TC_BUFFER) {

				//
				//	convert an octet to an string
				//
				cyng::buffer_t buffer;
				buffer = cyng::value_cast(obj, buffer);
				ss << cyng::io::to_ascii(buffer);
			}
			else {

				//
				//	everything else
				//
				cyng::io::serialize_plain(ss, obj);
			}
			return ss.str();
		}

	}
}