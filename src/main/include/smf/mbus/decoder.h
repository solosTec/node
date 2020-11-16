/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SML_MBUS_DECODER_H
#define NODE_SML_MBUS_DECODER_H

#include <smf/mbus/header.h>
#include <smf/sml/intrinsics/obis.h>

#include <cyng/intrinsics/buffer.h>
#include <crypto/aes.h>
#include <cyng/log.h>
#include <cyng/vm/controller_fwd.h>

#include <cstdint>
#include <array>

#include <boost/uuid/random_generator.hpp>

namespace node
{
	namespace mbus	
	{
		/**
		 * Decode payload
		 */
		class decoder
		{};
	}

	namespace wmbus
	{
		/**
		 * Decode payload
		 */
		class decoder
		{
		public:
			using cb_meter = std::function<void(cyng::buffer_t const& srv_id
				, std::uint8_t status
				, std::uint8_t aes_mode
				, cyng::crypto::aes_128_key const& aes)>;

			/**
			 * Callback for decoded data
			 */
			using cb_data = std::function<void(cyng::buffer_t const& srv_id
				, cyng::buffer_t const& data
				, std::uint8_t status
				, boost::uuids::uuid pk)>;

			/**
			 * Callback for values
			 */
			using cb_value = std::function<void(cyng::buffer_t const& srv_id
				, cyng::object const& val
				, std::uint8_t scaler
				, mbus::units unit
				, sml::obis code
				, boost::uuids::uuid pk)>;

		public:
			decoder(cyng::logging::log_ptr
				, cyng::controller& vm
				, cb_meter
				, cb_data
				, cb_value);

			bool run(cyng::buffer_t const& srv_id
				, std::uint8_t frame_type
				, cyng::buffer_t const& payload
				, cyng::crypto::aes_128_key const& aes);
		private:
			/*
			 * The first 12 bytes of the user data consist of a block with a fixed size and structure.
			 *
			 * Applied from master with CI = 0x53, 0x55, 0x5B, 0x5F, 0x60, 0x64, 0x6Ch, 0x6D
			 * Applied from slave with CI = 0x68, 0x6F, 0x72, 0x75, 0x7C, 0x7E, 0x9F
			 */
			bool read_frame_header_long(cyng::buffer_t const& srv_id
				, cyng::buffer_t const& payload
				, cyng::crypto::aes_128_key const& aes);

			/**
			 * Applied from slave with CI = 0x7A
			 */
			bool read_frame_header_short(cyng::buffer_t const& srv_id
				, cyng::buffer_t const& payload
				, cyng::crypto::aes_128_key const& aes);

			/**
			 * includes an SML parser
			 */
			bool read_frame_header_short_sml(cyng::buffer_t const& srv_id
				, cyng::buffer_t const& payload
				, cyng::crypto::aes_128_key const& aes);

			std::pair<cyng::buffer_t, bool> decode_data(cyng::buffer_t const& server_id
				, wireless_prefix const& prefix
				, cyng::crypto::aes_128_key const& aes);

			/**
			 * read VIFs and DIFs
			 */
			void read_variable_data_block(cyng::buffer_t const& server_id
				, std::uint8_t const blocks
				, cyng::buffer_t const& raw_data
				, boost::uuids::uuid);


		private:
			cyng::logging::log_ptr logger_;
			cyng::controller& vm_;
			cb_meter cb_meter_;
			cb_data cb_data_;
			cb_value cb_value_;
			boost::uuids::random_generator_mt19937 uuidgen_;
		};
	}
}


#endif
