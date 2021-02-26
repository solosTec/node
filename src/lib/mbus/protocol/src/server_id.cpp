/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/mbus/server_id.h>

#include <cyng/obj/buffer_cast.hpp>

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

		return cyng::to_buffer(id);
	}

}


