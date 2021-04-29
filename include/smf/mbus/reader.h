/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_READER_H
#define SMF_MBUS_READER_H

#include <smf/mbus/dif.h>
#include <smf/mbus/vif.h>

#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/object.h>

#include <cstdint>
#include <chrono>

namespace smf
{
	namespace mbus	
	{
		/**
		 * read a buffer from the specified offset and return the next vif
		 */
		std::size_t read(cyng::buffer_t const&, std::size_t offset);

		/**
		 * get the size of the value in bytes
		 */
		std::size_t calculate_size(data_field_code, char);

		/**
		 * convert 3 bytes to a date
		 */
		std::chrono::system_clock::time_point convert_to_tp(char, char, char, char);

		/**
		 * produce an integer value
		 */
		cyng::object make_i24_value(char, char, char);
	}
}


#endif
