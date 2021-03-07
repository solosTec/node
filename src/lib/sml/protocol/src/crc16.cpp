/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/crc16.h>
#include <boost/assert.hpp>

namespace smf
{
	namespace sml	
	{
		std::uint16_t crc16_calculate(const unsigned char *cp, std::size_t len)
		{
			std::uint16_t fcs = crc_init();	//	initial FCS value

			while (len--) {
				fcs = (fcs >> 8) ^ fcstab[(fcs ^ *cp++) & 0xff];
			}

			//	finalize
			fcs ^= 0xffff;
			fcs = ((fcs & 0xff) << 8) | ((fcs & 0xff00) >> 8);

			return fcs;
		}

		std::uint16_t crc16_calculate(cyng::buffer_t const& b)
		{
			BOOST_ASSERT_MSG(b.size() > 4, "insufficient buffer size");
			return (b.size() > 4)
				? crc16_calculate((const unsigned char*)b.data(), b.size() - 4)
				: 0
				;
		}

		std::uint16_t set_crc16(cyng::buffer_t& buffer)
		{
			BOOST_ASSERT_MSG(buffer.size() > 4, "message buffer to small");

			crc_16_data crc;
			crc.crc_ = crc16_calculate(buffer);

			if (buffer.size() > 4) 	{
				//	patch 
				buffer[buffer.size() - 3] = crc.data_[1];
				buffer[buffer.size() - 2] = crc.data_[0];
				BOOST_ASSERT_MSG(buffer[buffer.size() - 1] == 0, "invalid EOD");
			}
			return crc.crc_;
		}

	}
}
