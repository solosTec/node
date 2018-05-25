/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_SML_SERIALIZER_H
#define NODE_LIB_SML_SERIALIZER_H

#include <cyng/intrinsics/buffer.h>
#include <cyng/intrinsics/sets.h>
#include <ostream>

namespace node
{
	namespace sml
	{
		/**
		 * Serialize an object into a stream of SML data
		 */
		void serialize(std::ostream&, cyng::object);
		void serialize(std::ostream&, cyng::tuple_t);

		/**
		 * Write a tuple of SML data in a buffer
		 */
		cyng::buffer_t linearize(cyng::tuple_t);
	}
}
#endif