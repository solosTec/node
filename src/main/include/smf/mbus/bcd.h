/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SML_MBUS_BCD_H
#define NODE_SML_MBUS_BCD_H

#include <cyng/intrinsics/buffer.h>
#include <cstdint>
#include <algorithm>
#include <boost/assert.hpp>
#include <boost/range/adaptor/reversed.hpp>

namespace node
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
