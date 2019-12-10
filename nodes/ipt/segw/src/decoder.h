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
#include <boost/uuid/uuid.hpp>

namespace node
{
	class cache;
	class header_long;
	class header_short;
	class decoder_wired_mbus
	{
	public:
		decoder_wired_mbus(cyng::logging::log_ptr);

	private:
		cyng::logging::log_ptr logger_;
	};

	class decoder_wireless_mbus
	{
	public:
		decoder_wireless_mbus(cyng::logging::log_ptr, cache& cfg, boost::uuids::uuid tag);

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
		bool decode_data(std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_long& hl);
		bool decode_data(std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_short&);
		bool decode_data_mode_5(std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_long&);
		bool decode_data_mode_5(std::uint8_t aes_mode, cyng::buffer_t const& server_id, header_short&);

		void read_variable_data_block(cyng::buffer_t const& server_id, header_short const& hs);

	private:
		cyng::logging::log_ptr logger_;
		cache& cache_;
		boost::uuids::uuid const tag_;
	};

}

#endif
