/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */


#include <smf/sml/srv_id_io.h>
#include <smf/sml/defs.h>
#include <smf/sml/parser/srv_id_parser.h>

#include <cyng/io/io_buffer.h>
#include <cyng/parser/buffer_parser.h>

#include <iomanip>
#include <sstream>
#include <algorithm>

#include <boost/io/ios_state.hpp>

namespace node
{
	namespace sml
	{
		bool is_mbus_wired(cyng::buffer_t const& buffer)
		{
			return ((buffer.size() == 9) && (buffer.at(0) == MBUS_WIRED));
		}

		bool is_mbus_radio(cyng::buffer_t const& buffer)
		{
			return ((buffer.size() == 9) && (buffer.at(0) == MBUS_WIRELESS));
		}

		bool is_w_mbus(cyng::buffer_t const& buffer)
		{
			//	Each meter (or radio module connected to the meter) has a unique 8 byte ID or address. 
			//	The address consists of 
			//	* a Manufacturer ID (2 Bytes), 
			//	* a serial number (4 Bytes), 
			//	* the device type (e.g. water meter) (1 Byte) and 
			//	* the version of the meter (product revision). (1 Bytes)
			if (buffer.size() == 8)
			{
				return (buffer.at(6) >= 0x00) && (buffer.at(6) <= 0x0F);
			}
			return false;
		}

		bool is_serial(cyng::buffer_t const& buffer)
		{
			return std::all_of(buffer.begin(), buffer.end(), [](char c) {
				return ((c >= '0') && (c <= '9'))
					|| ((c >= 'A') && (c <= 'Z'));
			});
		}

		bool is_gateway(cyng::buffer_t const& buffer)
		{
			return ((buffer.size() == 7) && (buffer.at(0) == MAC_ADDRESS));
		}

		bool is_dke_1(cyng::buffer_t const& buffer)
		{
			return (buffer.size() == 10) 
				&& (buffer.at(0) == '0') 
				&& (buffer.at(1) == '6');
		}

		bool is_dke_2(cyng::buffer_t const& buffer)
		{
			return (buffer.size() == 10)
				&& (buffer.at(0) == '0')
				&& (buffer.at(1) == '9');
		}

		bool is_switch(cyng::buffer_t const& buffer)
		{
			return (buffer.size() == 4)
				&& (buffer.at(0) == 10);
		}

		srv_type get_srv_type(cyng::buffer_t const& buffer)
		{
			if (is_w_mbus(buffer))	return SRV_W_MBUS;
			else if (is_mbus_wired(buffer))	return SRV_MBUS_WIRED;
			else if (is_mbus_radio(buffer))	return SRV_MBUS_RADIO;
			else if (is_gateway(buffer))	return SRV_GW;
			else if (buffer.size() == 10 && buffer.at(1) == '3')	return SRV_BCD;
			else if (buffer.size() == 8 && buffer.at(2) == '4')	return SRV_EON;
			else if (is_dke_1(buffer))	return SRV_DKE_1;
			else if (is_dke_2(buffer))	return SRV_DKE_2;
			else if (is_switch(buffer))	return SRV_SWITCH;
			else if (is_serial(buffer))	return SRV_SERIAL;

			return SRV_OTHER;
		}

		bool is_mbus(std::string const& str)
		{
			//example: 02-e61e-03197715-3c-07
			const auto size = str.size();
			if (size == 22 
				&& (str.at(0) == '0')
				&& ((str.at(1) == '1') || (str.at(1) == '2'))
				&& (str.at(2) == '-')
				&& (str.at(7) == '-')
				&& (str.at(16) == '-')
				&& (str.at(19) == '-')) {

				//
				//	only hex values allowed
				//
				return std::all_of(str.begin(), str.end(), [](char c) {
					return ((c >= '0') && (c <= '9'))
						|| ((c >= 'a') && (c <= 'f'))
						|| ((c >= 'A') && (c <= 'F'))
						|| ((c == '-'))
						;
				});
			}
			return false;
		}

		bool is_serial(std::string const& str)
		{
			//	example: 05823740
			const auto size = str.size();
			if (size == 8) {

				//
				//	only decimal values allowed
				//
				return std::all_of(str.begin(), str.end(), [](char c) {
					return ((c >= '0') && (c <= '9'));
				});
			}
			return false;
		}

		void serialize_server_id(std::ostream& os, cyng::buffer_t const& buffer)
		{
			//	store and reset stream state
			boost::io::ios_flags_saver  ifs(os);

			if (is_mbus_wired(buffer) || is_mbus_radio(buffer))
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
				//	something else like 31454d4830303035353133383935 (1EMH0005513895)
				//	

				//
				if ((buffer.size() > 10) && cyng::is_ascii(buffer)) {
					cyng::io::to_ascii(os, buffer);
				}
				else {
					cyng::io::to_hex(os, buffer);
				}
			}
		}

		std::string from_server_id(cyng::buffer_t const& buffer)
		{
			std::ostringstream ss;
			serialize_server_id(ss, buffer);
			return ss.str();
		}

		cyng::buffer_t from_server_id(std::string const& id)
		{
			auto const r = parse_srv_id(id);
			return (r.second)
				? r.first
				: cyng::buffer_t()
				;
		}

		std::string get_serial(cyng::buffer_t const& buffer)
		{
			if (is_mbus_wired(buffer) || is_mbus_radio(buffer))
			{
				//	serial number is encoded in bytes 6, 5, 4, 3
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
			else if (is_w_mbus(buffer)) {
				//	serial number is encoded in bytes 2, 3, 4, 5
				std::stringstream ss;
				ss
					<< std::hex
					<< std::setfill('0')
					<< std::setw(2)
					<< (+buffer.at(2) & 0xFF)
					<< std::setw(2)
					<< (+buffer.at(3) & 0xFF)
					<< std::setw(2)
					<< (+buffer.at(4) & 0xFF)
					<< std::setw(2)
					<< (+buffer.at(5) & 0xFF)
					;
				return ss.str();

			}
			else if (is_serial(buffer))
			{
				return cyng::io::to_ascii(buffer);
			}
			else if (buffer.size() == 4) {

				return cyng::io::to_hex(buffer);
			}

			return "00000000";
		}

		std::string get_serial(std::string const& str)
		{
			if (is_mbus(str))
			{
				//example: 02-e61e-03197715-3c-07
				return str.substr(14, 2)
					+ str.substr(12, 2)
					+ str.substr(10, 2)
					+ str.substr(8, 2)
					;
			}
			else if (is_serial(str))
			{
				return str;
			}

			return str;
		}

		std::uint16_t get_manufacturer_code(std::string const& str)
		{
			try {
				if (is_mbus(str)) {
					const auto hex = str.substr(3, 2);
					return static_cast<std::uint16_t>(std::stoul(hex, nullptr, 16));
				}
			}
			catch (std::exception const&) {
				//	std::invalid_argument 
				//	std::out_of_range
			}
			return 0u;
		}

		std::uint16_t get_manufacturer_code(cyng::buffer_t const& buffer)
		{
			if (is_mbus_wired(buffer) || is_mbus_radio(buffer)) {
				std::uint16_t code{ 0 };
				code = buffer[1] & 0xFF;
				code += static_cast<unsigned char>(buffer[2]) << 8;
				return code;
			}
			else if (is_w_mbus(buffer)) {
				//	manufacturer is encoded in bytes 0 .. 1
				std::uint16_t code{ 0 };
				code = buffer[0] & 0xFF;
				code += static_cast<unsigned char>(buffer[1]) << 8;
				return code;
			}
			return 0u;
		}

		std::uint8_t get_medium_code(cyng::buffer_t const& buffer)
		{
			return (buffer.size() == 9)
				? static_cast<std::uint8_t>(buffer.at(8))
				: 0u
				;
		}

		std::uint8_t get_version(cyng::buffer_t const& buffer)
		{
			return (buffer.size() == 9)
				? static_cast<std::uint8_t>(buffer.at(7))
				: 0u
				;
		}


		cyng::buffer_t to_gateway_srv_id(cyng::mac48 mac)
		{
			cyng::buffer_t buffer(mac.to_buffer());
			buffer.insert(buffer.begin(), 0x05);
			return buffer;
		}

		cyng::mac48 to_mac48(cyng::buffer_t const& buf)
		{
			return ((buf.size() == 7) && (buf.front() == 0x05))
				? cyng::mac48(buf.at(5), buf.at(4), buf.at(3), buf.at(2), buf.at(1), buf.at(0))
				: cyng::mac48()
				;
		}

		cyng::mac48 to_mac48(std::string const& str)
		{
			if ((str.size() == 14) && (str.at(0) == '0') && (str.at(1) == '5')) {
				auto const r = cyng::parse_hex_string_safe(str);
				if (r.second) {
					return to_mac48(r.first);
				}
			}
			return cyng::mac48();
		}


	}
}