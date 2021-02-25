/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_BCD_HPP
#define SMF_MBUS_BCD_HPP

#include <cyng/obj/intrinsics/buffer.h>
#include <cstdint>
#include <algorithm>
#include <boost/assert.hpp>
#include <boost/range/adaptor/reversed.hpp>

namespace smf
{
	namespace mbus	
	{
		template <typename T>
		T bcd_to_n(cyng::buffer_t const& buffer) {

			T n{ T() };

			//
			//	must start from the end
			//
			for (auto c : boost::adaptors::reverse(buffer)) {
				n *= 100u;
				n += T((c >> 4) & 0x0F) * 10u;
				n += T(c & 0x0F);
			}
			return n;
		}
	}
}

#endif
