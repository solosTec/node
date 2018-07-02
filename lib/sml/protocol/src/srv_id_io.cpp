/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */


#include <smf/sml/srv_id_io.h>
#include <smf/sml/defs.h>
#include <cyng/io/io_buffer.h>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <boost/io/ios_state.hpp>

namespace node
{
	namespace sml
	{
		bool is_mbus(cyng::buffer_t const& buffer)
		{
			return ((buffer.size() == 9)
				&& (buffer.at(0) == MBUS_WIRELESS || buffer.at(0) == MBUS_WIRED));
		}

		bool is_serial(cyng::buffer_t const& buffer)
		{
			if (buffer.size() == 8)
			{
				return std::all_of(buffer.begin(), buffer.end(), [](char c) {
					return ((c >= '0') && (c <= '9'))
						|| ((c >= 'A') && (c <= 'Z'));
				});
			}
			return false;
		}

		bool is_gateway(cyng::buffer_t const& buffer)
		{
			return ((buffer.size() == 7) && (buffer.at(0) == MAC_ADDRESS));
		}

		void serialize_server_id(std::ostream& os, cyng::buffer_t const& buffer)
		{
			//	store and reset stream state
			boost::io::ios_flags_saver  ifs(os);

			if (is_mbus(buffer))
			{
				//
				//	serial number with M-bus prefix
				//	example: 01-e61e-13090016-3c-07
				//
				os
					<< std::hex
					<< std::setfill('0')
					;

				std::size_t counter{ 0 };
				for (auto c : buffer)
				{ 
					os << std::setw(2) << (+c & 0xFF);
					counter++;
					switch (counter)
					{
					case 1: case 3: case 7: case 8:
						os << '-';
						break;
					default:
						break;
					}
				}
			}
			else if (is_serial(buffer))
			{
				//
				//	serial number ASCII encoded
				//
				os << cyng::io::to_ascii(buffer);
			}
			else if (is_gateway(buffer))
			{
				//
				//	MAC as serial number
				//
				os
					<< std::hex
					<< std::setfill('0')
					;

				//	skip leading 0x05
				std::size_t counter{ 0 };
				for (auto c : buffer)
				{
					if (counter > 0) {
						if (counter > 1) {
							os << ':';
						}
						os << std::setw(2) << (+c & 0xFF);
					}
					++counter;
				}
			}
			else if (buffer.size() == 8)
			{
				//
				//	serial number without prefix
				//
				//	e.g. 2d2c688668691c04 => 2d2c-68866869-1c-04
				os
					<< std::hex
					<< std::setfill('0')
					;

				std::size_t counter{ 0 };
				for (auto c : buffer)
				{
					os << std::setw(2) << (+c & 0xFF);
					counter++;
					switch (counter)
					{
					case 2: case 6: case 7:
						os << '-';
						break;
					default:
						break;
					}
				}
			}
			else
			{
				//
				//	something else
				//
				os
					<< std::hex
					<< std::setfill('0')
					;
				for (auto c : buffer)
				{
					os << std::setw(2) << (+c & 0xFF);
				}
			}
		}

		std::string from_server_id(cyng::buffer_t const& buffer)
		{
			std::ostringstream ss;
			serialize_server_id(ss, buffer);
			return ss.str();
		}

		std::string get_serial(cyng::buffer_t const& buffer)
		{
			if (is_mbus(buffer))
			{
				std::stringstream ss;
				ss
					<< std::hex
					<< std::setfill('0')
					<< std::setw(2)
					<< (+buffer.at(6) & 0xFF)
					<< std::setw(2)
					<< (+buffer.at(5) & 0xFF)
					<< std::setw(2)
					<< (+buffer.at(4) & 0xFF)
					<< std::setw(2)
					<< (+buffer.at(3) & 0xFF)
					;
				return ss.str();
			}
			else if (is_serial(buffer))
			{
				return cyng::io::to_ascii(buffer);
			}

			return "00000000";
		}

		cyng::buffer_t to_gateway_srv_id(cyng::mac48 mac)
		{
			cyng::buffer_t buffer(mac.to_buffer());
			buffer.insert(buffer.begin(), 0x05);
			return buffer;
		}

	}
}