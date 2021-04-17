/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_RADIO_DECODE_H
#define SMF_MBUS_RADIO_DECODE_H

/** @file decode.h
 * To decode data use the decryp() function from the smfsec project
 */
#include <smf/mbus/radio/header.h>

#include <smfsec/aes.h>

#include <boost/assert.hpp>

namespace smf
{
	namespace mbus	
	{
		namespace radio
		{
			/**
			 * build initial vector
			 */
			cyng::crypto::aes::iv_t build_initial_vector(srv_id_t const& srv_id, std::uint8_t access_nr);

			/**
			 * decode
			 */
			cyng::buffer_t decode(srv_id_t const& srv_id
				, std::uint8_t access_nr
				, cyng::crypto::aes_128_key key
				, cyng::buffer_t const&);
		}
	}
}


#endif
