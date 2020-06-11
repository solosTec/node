/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_SEGW_DECODER_H
#define NODE_IPT_SEGW_DECODER_H

#include <cyng/intrinsics/buffer.h>
#include <cyng/log.h>
#include <cyng/vm/controller_fwd.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	class cache;
	class wireless_prefix;
	class header_long_wireless;
	class header_long_wired;

	class decoder_wired_mbus
	{
	public:
		decoder_wired_mbus(cyng::logging::log_ptr
			, cache& cfg
			, cyng::controller&);

		bool read_short_frame(std::uint8_t,		//	C-field
			std::uint8_t,		//	A-field
			std::uint8_t,		//	checksum
			cyng::buffer_t const&);
		bool read_long_frame(std::uint8_t,		//	C-field
			std::uint8_t,		//	A-field
			std::uint8_t,		//	CI-field
			std::uint8_t,		//	checksum
			cyng::buffer_t const&);
		bool read_ctrl_frame(std::uint8_t,		//	C-field
			std::uint8_t,		//	A-field
			std::uint8_t,		//	CI-field
			std::uint8_t);		//	checksum

	private:
		bool read_frame_header_long(cyng::buffer_t const&);
		void read_variable_data_block(cyng::buffer_t const&
			, header_long_wired const&
			, boost::uuids::uuid);

	private:
		cyng::logging::log_ptr logger_;
		cache& cache_;
		cyng::controller& vm_;
		boost::uuids::random_generator_mt19937 uuidgen_;
	};

	class decoder_wireless_mbus
	{
	public:
		decoder_wireless_mbus(cyng::logging::log_ptr
			, cache& cfg
			, cyng::controller&);

		/**
	     * The first 12 bytes of the user data consist of a block with a fixed length and structure.
	     *
		 * Applied from master with CI = 0x53, 0x55, 0x5B, 0x5F, 0x60, 0x64, 0x6Ch, 0x6D
		 * Applied from slave with CI = 0x68, 0x6F, 0x72, 0x75, 0x7C, 0x7E, 0x9F
		 */
		bool read_frame_header_long(cyng::buffer_t const&, cyng::buffer_t const&);

		/**
		 * Applied from master with CI = 0x7A
		 */
		bool read_frame_header_short(cyng::buffer_t const&, cyng::buffer_t const&);

		/**
		 * frame type 0x7F
		 */
		bool read_frame_header_short_sml(cyng::buffer_t const&, cyng::buffer_t const&);

	private:
		std::pair<cyng::buffer_t, bool> decode_data(cyng::buffer_t const& server_id
			, wireless_prefix const&);

		std::pair<cyng::buffer_t, bool> decode_data_mode_5(cyng::buffer_t const& server_id
			, wireless_prefix const&);

		void read_variable_data_block(cyng::buffer_t const& server_id
			, std::uint8_t const blocks
			, cyng::buffer_t const& raw_data
			, boost::uuids::uuid pk);

	private:
		cyng::logging::log_ptr logger_;
		cache& cache_;
		cyng::controller& vm_;
		boost::uuids::random_generator_mt19937 uuidgen_;
	};

}

#endif
