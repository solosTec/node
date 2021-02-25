/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_AES_H
#define SMF_MBUS_AES_H

#include <cyng/obj/intrinsics/buffer.h>

#include <crypto/aes.h>

#include <cstdint>
#include <array>

namespace smf
{
	namespace mbus	
	{
		/**
		 * Build AES CBC initial vector  according to FIPS 197.
		 * buffer has to be in M-Bus format
		 */
		cyng::crypto::aes::iv_t build_initial_vector(cyng::buffer_t const& buffer, std::uint8_t access_nr);
		cyng::buffer_t build_initial_vector(std::array<std::uint8_t, 16> const&);

	}
}


#endif
