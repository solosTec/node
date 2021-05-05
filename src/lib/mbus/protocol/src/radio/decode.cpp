/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/mbus/radio/decode.h>

#include <iomanip>
#include <sstream>
#include <cstring>

#include <boost/assert.hpp>

namespace smf
{
	namespace mbus
	{
		namespace radio
		{
			cyng::crypto::aes::iv_t build_initial_vector(srv_id_t const& srv_id, std::uint8_t access_nr) {

				cyng::crypto::aes::iv_t iv{ 0 };
				iv.fill(access_nr);

				//
				//	M-field + A-field
				//
				auto pos = srv_id.begin();
				++pos;
				std::copy(pos, srv_id.end(), iv.begin());

				BOOST_ASSERT(iv.at(8) == access_nr);
				BOOST_ASSERT(iv.at(0xf) == access_nr);

				return iv;
			}

			cyng::buffer_t decode(srv_id_t const& srv_id
				, std::uint8_t access_nr
				, cyng::crypto::aes_128_key key
				, cyng::buffer_t const& data) {

				//
				//	build iv and decrypt data
				//
				return cyng::crypto::aes::decrypt(data, key, build_initial_vector(srv_id, access_nr));
			}

			bool is_decoded(cyng::buffer_t const& buffer) {

				if (buffer.size() > 2) {
					return (buffer.at(0) == static_cast<char>(0x2f))
						&& (buffer.at(1) == static_cast<char>(0x2f))
						;
				}
				return false;

			}

		}
	}
}


