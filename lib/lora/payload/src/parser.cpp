/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/lora/payload/parser.h>
#include <smf/sml/mbus/defs.h>
#include <smf/sml/scaler.h>
#include <cyng/parser/buffer_parser.h>

#include <bitset>
#include <boost/assert.hpp>
#include <boost/crc.hpp>      // for boost::crc_basic, boost::crc_optimal
#include <iomanip>
#ifdef _DEBUG
#include <iostream> 
#endif
#include <sstream>

namespace node
{
	namespace lora
	{
		cyng::buffer_t parse_payload(std::string const& inp)
		{
			//BOOST_ASSERT_MSG(inp.size() == PAYLOAD_SIZE * 2, "wrong input size");
			if (inp.size() == PAYLOAD_SIZE * 2)
			{
				const std::pair<cyng::buffer_t, bool > r = cyng::parse_hex_string(inp);
				if (r.second)
				{
					return r.first;
				}
			}

			//	create an empty buffer
			return cyng::buffer_t();
		}

		std::uint8_t version(cyng::buffer_t const& buf)
		{
			return (buf.size() == PAYLOAD_SIZE)
				? buf[0]
				: 0
				;
		}

		std::string	manufacturer(cyng::buffer_t const& buf)
		{
			if (buf.size() == PAYLOAD_SIZE)
			{
				return node::sml::decode(buf[1], buf[2]);
			}
			return "???";
		}

		std::string	meter_id(cyng::buffer_t const& buf)
		{
			if (buf.size() == PAYLOAD_SIZE)
			{
				std::ostringstream ss;
				ss
					<< std::hex
					<< std::setfill('0')
					<< std::setw(2)
					<< (static_cast<unsigned>(buf.at(6)) & 0xFF)
					<< std::setw(2)
					<< (static_cast<unsigned>(buf.at(5)) & 0xFF)
					<< std::setw(2)
					<< (static_cast<unsigned>(buf.at(4)) & 0xFF)
					<< std::setw(2)
					<< (static_cast<unsigned>(buf.at(3)) & 0xFF)
					;
				return ss.str();
			}

			return "00000000";
		}

		cyng::buffer_t server_id(cyng::buffer_t const& buf)
		{
			return (buf.size() == PAYLOAD_SIZE)
				? cyng::buffer_t({ buf.at(6), buf.at(5), buf.at(4), buf.at(3) })
				: cyng::buffer_t({ 0, 0, 0, 0 })
				;
		}


		std::uint8_t medium(cyng::buffer_t const& buf)
		{
			return (buf.size() == PAYLOAD_SIZE)
				? buf[7]
				: 0
				;
		}

		std::uint8_t state(cyng::buffer_t const& buf)
		{
			return (buf.size() == PAYLOAD_SIZE)
				? buf[8]
				: 0
				;
		}

		std::uint16_t actuality(cyng::buffer_t const& buf)
		{
			return (buf.size() == PAYLOAD_SIZE)
				? * reinterpret_cast<std::uint16_t const*>(buf.data() + 9)
				: 0
				;
		}

		std::uint8_t vif(cyng::buffer_t const& buf)
		{
			return (buf.size() == PAYLOAD_SIZE)
				? buf[11]
				: 0
				;
		}

		std::uint32_t volume(cyng::buffer_t const& buf)
		{
			return (buf.size() == PAYLOAD_SIZE)
				? * reinterpret_cast<std::uint32_t const*>(buf.data() + 12)
				: 0
				;
		}

		std::string	value(std::uint8_t vif, std::uint32_t volume)
		{
			return node::sml::scale_value(volume, scaler(vif));
		}

		std::int8_t	scaler(std::uint8_t vif)
		{
			BOOST_ASSERT_MSG((vif > 0x9) && (vif < 0x17), "VIF out of range");
			return ((vif > 0x9) && (vif < 0x17))
				? ((0x16 - vif) * -1)
				: 0
				;
		}

		std::uint8_t functions(cyng::buffer_t const& buf)
		{
			return (buf.size() == PAYLOAD_SIZE)
				? buf[16]
				: 0
				;
		}

		std::uint8_t lifetime(cyng::buffer_t const& buf)
		{
			return (buf.size() == PAYLOAD_SIZE)
				? buf[17]
				: 0
				;
		}

		std::uint32_t semester(std::uint8_t v)
		{
			const std::uint32_t s = (v >> 3) & 0x1f;
			return s;
		}

		bool link_error(cyng::buffer_t const& buf)
		{
			static const std::bitset<8> bs(4);
			return (buf.size() == PAYLOAD_SIZE)
				? ((lifetime(buf) & 4) == 4)
				: false
				;
		}
		
		std::uint16_t crc(cyng::buffer_t const& buf)
		{
			return (buf.size() == PAYLOAD_SIZE)
				? * reinterpret_cast<std::uint16_t const*>(buf.data() + 18)
				: 0
				;
		}

		std::uint16_t crc_calculated(cyng::buffer_t const& buf)
		{
			if (buf.size() == PAYLOAD_SIZE)
			{
				boost::crc_ccitt_type result;
				result.process_bytes(buf.data(), PAYLOAD_SIZE - 2);
				return result.checksum();
			}
			return 0;
		}

		bool crc_ok(cyng::buffer_t const& buf)
		{
			return crc(buf) == crc_calculated(buf);
		}


	}	//	json
}

