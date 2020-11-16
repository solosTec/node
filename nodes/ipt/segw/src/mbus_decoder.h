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

}

#endif
