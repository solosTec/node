/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/protocol/serializer.h>
#include <smf/sml/protocol/value.hpp>

#include "writer.hpp"
#include <cyng/object.h>
#include <cyng/intrinsics/traits/tag.hpp>
#include <cyng/object_cast.hpp>

namespace node
{
	namespace sml
	{
		template <typename T>
		void do_write(std::ostream& os, cyng::object const& obj)
		{
			using serial_t = serializer <T>;
			auto p = cyng::object_cast<T>(obj);
			BOOST_ASSERT_MSG(p != nullptr, "cannot serialize null pointer");
			if (p != nullptr)	serial_t::write(os, *p);
			else os.put(0x01);	//	[OPTIONAL]
		}

		void serialize(std::ostream& os, cyng::object obj)
		{
			switch (obj.get_class().tag())
			{
			case cyng::TC_NULL:
				os.put(0x01);
				//do_write<typename std::tuple_element<cyng::TC_NULL, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_EOD:
				os.put(0x00);
				//do_write<typename std::tuple_element<cyng::TC_EOD, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_BOOL:
				do_write<typename std::tuple_element<cyng::TC_BOOL, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_CHAR:
				do_write<typename std::tuple_element<cyng::TC_CHAR, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_FLOAT:
				do_write<typename std::tuple_element<cyng::TC_FLOAT, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_DOUBLE:
				do_write<typename std::tuple_element<cyng::TC_DOUBLE, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_UINT8:
				do_write<typename std::tuple_element<cyng::TC_UINT8, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_UINT16:
				do_write<typename std::tuple_element<cyng::TC_UINT16, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_UINT32:
				do_write<typename std::tuple_element<cyng::TC_UINT32, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_UINT64:
				do_write<typename std::tuple_element<cyng::TC_UINT64, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_INT8:
				do_write<typename std::tuple_element<cyng::TC_INT8, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_INT16:
				do_write<typename std::tuple_element<cyng::TC_INT16, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_INT32:
				do_write<typename std::tuple_element<cyng::TC_INT32, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_INT64:
				do_write<typename std::tuple_element<cyng::TC_INT64, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_STRING:
				do_write<typename std::tuple_element<cyng::TC_STRING, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_TIME_POINT:
				do_write<typename std::tuple_element<cyng::TC_TIME_POINT, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_NANO_SECOND:
				do_write<typename std::tuple_element<cyng::TC_NANO_SECOND, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_MICRO_SECOND:
				do_write<typename std::tuple_element<cyng::TC_MICRO_SECOND, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_MILLI_SECOND:
				do_write<typename std::tuple_element<cyng::TC_MILLI_SECOND, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_SECOND:
				do_write<typename std::tuple_element<cyng::TC_SECOND, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_MINUTE:
				do_write<typename std::tuple_element<cyng::TC_MINUTE, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_HOUR:
				do_write<typename std::tuple_element<cyng::TC_HOUR, cyng::traits::tag_t>::type>(os, obj);
				break;
				// 
			case cyng::TC_DBL_TP:
				do_write<typename std::tuple_element<cyng::TC_DBL_TP, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_VERSION:
				do_write<typename std::tuple_element<cyng::TC_VERSION, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_REVISION:
				do_write<typename std::tuple_element<cyng::TC_REVISION, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_CODE:
				do_write<typename std::tuple_element<cyng::TC_CODE, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_BUFFER:
				do_write<typename std::tuple_element<cyng::TC_BUFFER, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_MAC48:
				do_write<typename std::tuple_element<cyng::TC_MAC48, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_MAC64:
				do_write<typename std::tuple_element<cyng::TC_MAC64, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_COLOR_8:
				do_write<typename std::tuple_element<cyng::TC_COLOR_8, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_COLOR_16:
				do_write<typename std::tuple_element<cyng::TC_COLOR_16, cyng::traits::tag_t>::type>(os, obj);
				break;

				// 			
			case cyng::TC_TUPLE:
				do_write<typename std::tuple_element<cyng::TC_TUPLE, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_VECTOR:
				do_write<typename std::tuple_element<cyng::TC_VECTOR, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_SET:
				do_write<typename std::tuple_element<cyng::TC_SET, cyng::traits::tag_t>::type>(os, obj);
				break;
				// 			// 
			case cyng::TC_ATTR_MAP:
				do_write<typename std::tuple_element<cyng::TC_ATTR_MAP, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_ATTR:
				do_write<typename std::tuple_element<cyng::TC_ATTR, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_PARAM_MAP:
				do_write<typename std::tuple_element<cyng::TC_PARAM_MAP, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_PARAM:
				do_write<typename std::tuple_element<cyng::TC_PARAM, cyng::traits::tag_t>::type>(os, obj);
				break;

			case cyng::TC_EC:
				do_write<typename std::tuple_element<cyng::TC_EC, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_UUID:
				do_write<typename std::tuple_element<cyng::TC_UUID, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_FS_PATH:
				do_write<typename std::tuple_element<cyng::TC_FS_PATH, cyng::traits::tag_t>::type>(os, obj);
				break;

			case cyng::TC_IP_TCP_ENDPOINT:
				do_write<typename std::tuple_element<cyng::TC_IP_TCP_ENDPOINT, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_IP_UDP_ENDPOINT:
				do_write<typename std::tuple_element<cyng::TC_IP_UDP_ENDPOINT, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_IP_ICMP_ENDPOINT:
				do_write<typename std::tuple_element<cyng::TC_IP_ICMP_ENDPOINT, cyng::traits::tag_t>::type>(os, obj);
				break;
			case cyng::TC_IP_ADDRESS:
				do_write<typename std::tuple_element<cyng::TC_IP_ADDRESS, cyng::traits::tag_t>::type>(os, obj);
				break;

			//
			//	ToDo: obis 
			//

			default:
				std::cerr << "unknown type code: " << obj.get_class().tag() << ", " << obj.get_class().type_name() << std::endl;
				//do_write_custom<S>(os, obj);
				break;
			}
		}
	
		void serialize(std::ostream& os, cyng::tuple_t tpl)
		{
			using serial_t = serializer <cyng::tuple_t>;
			serial_t::write(os, tpl);
		}

		cyng::buffer_t linearize(cyng::tuple_t tpl)
		{
			//	temporary buffer
			boost::asio::streambuf stream_buffer;
			std::ostream os(&stream_buffer);

			//	serialize into a stream
			serialize(os, tpl);

			//	get content of buffer
			boost::asio::const_buffer cbuffer(*stream_buffer.data().begin());
			const char* p = boost::asio::buffer_cast<const char*>(cbuffer);
			const std::size_t size = boost::asio::buffer_size(cbuffer);

			//	create buffer object
			return cyng::buffer_t(p, p + size);
		}
	}
}
