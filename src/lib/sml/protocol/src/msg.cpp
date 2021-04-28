#include <smf/sml/msg.h>
#include <smf/sml/crc16.h>

#include <algorithm>
#ifdef _DEBUG_SML
#include <iostream>
#endif

#include <boost/assert.hpp>

namespace smf {
	namespace sml {

		cyng::buffer_t to_sml(sml::messages_t const& reqs)
		{
			std::vector<cyng::buffer_t>	msg;
			for (auto const& req : reqs) {
				//
				//	linearize and set CRC16
				//
				//cyng::buffer_t b = linearize(req);
				//sml_set_crc16(b);

				//
				//	append to current SML message
				//
				//msg.push_back(b);
			}

			return boxing(msg);
		}


		cyng::buffer_t boxing(std::vector<cyng::buffer_t> const& inp)
		{
			//
			//	trailer v1.0
			//
			cyng::buffer_t buf{ 0x1b, 0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01 };

			//
			//	append messages
			//
			for (auto const& msg : inp) {
				buf.insert(buf.end(), msg.begin(), msg.end());
			}

			//
			//	padding bytes
			//
			const char pad = ((buf.size() % 4) == 0)
				? 0
				: (4 - (buf.size() % 4))
				;
			for (std::size_t pos = 0; pos < pad; ++pos)	{
				buf.push_back(0x0);
			}

			//
			//	tail v1.0
			//
			buf.insert(buf.end(), { 0x1b, 0x1b, 0x1b, 0x1b, 0x1a, pad });


			//
			//	CRC calculation over complete buffer
			//
			crc_16_data crc;
			BOOST_ASSERT(buf.size() < std::numeric_limits<int>::max());
			crc.crc_ = crc16_calculate(reinterpret_cast<const unsigned char*>(buf.data()), buf.size());

			//	network order
			buf.push_back(crc.data_[1]);
			buf.push_back(crc.data_[0]);

			return buf;
		}

	}
}
