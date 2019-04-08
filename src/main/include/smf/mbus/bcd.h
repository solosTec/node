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

namespace node
{
	namespace mbus	
	{
		template <typename T>
		T bcd_to_n(cyng::buffer_t const& buffer) {

			BOOST_ASSERT_MSG(sizeof(T) == buffer.size(), "wrong buffer size");

			T n{ 0u };
			cyng::buffer_t tmp(buffer.size());

			//
			//	must start from the end
			//
			std::reverse_copy(buffer.begin(), buffer.end(), tmp.begin());
			for (auto c : tmp) {
				n *= 100u;
				//std::cout << unsigned((c >> 4) & 0x0F) << std::endl;
				//std::cout << unsigned(c & 0x0F) << std::endl;
				n += T((c >> 4) & 0x0F) * 10u;
				n += T(c & 0x0F);
			}
			return n;
		}
	}
}

#endif
