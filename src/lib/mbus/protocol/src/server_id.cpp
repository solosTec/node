/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/mbus/server_id.h>

#include <cyng/obj/buffer_cast.hpp>
#include <cyng/parse/hex.h>

#include <iomanip>
#include <sstream>
#include <cstring>

#include <boost/assert.hpp>

namespace smf
{
	cyng::buffer_t to_meter_id(std::uint32_t id)
	{
		//	Example: 0x3105c = > 96072000

		std::stringstream ss;
		ss.fill('0');
		ss
			<< std::setw(8)
			<< std::setbase(10)
			<< id;

		auto const s = ss.str();
		BOOST_ASSERT(s.size() == 8);

		ss
			>> std::setbase(16)
			>> id;

		return cyng::to_buffer_be(id);
	}

	cyng::buffer_t to_meter_id(std::string id) {

		//	Example: "10320047" => [10, 32, 00, 47]dec == [0A, 20, 00, 2F]hex
		BOOST_ASSERT(id.size() == 8);

		cyng::buffer_t r;
		std::fill_n(std::back_inserter(r), 4, 0);
		if (id.size() == 8) {
			for (std::size_t idx = 0; idx < id.size(); idx += 2) {
				r.at(idx / 2) = std::stoul(id.substr(idx, 2));
				//r.at(idx / 2) = cyng::hex_to_u8(id.at(idx), id.at(idx + 1));
			}
		}
		return r;
	}

	std::string srv_id_to_str(std::array<char, 9> a) {
		return srv_id_to_str(cyng::buffer_t(a.begin(), a.end()));
	}

	std::string srv_id_to_str(cyng::buffer_t id) {
		std::ostringstream ss;
		if (id.size() == 9) {
			ss
				<< std::hex
				<< std::setfill('0')
				;
			std::size_t counter{ 0 };
			for (auto c : id)
			{
				ss << std::setw(2) << (+c & 0xFF);
				counter++;
				switch (counter)
				{
				case 1: case 3: case 7: case 8:
					ss << '-';
					break;
				default:
					break;
				}
			}
		}
		else {
			//	all other cases
			for (auto c : id) {
				ss << std::setw(2) << (+c & 0xFF);
			}
		}
		return ss.str();
	}

}


