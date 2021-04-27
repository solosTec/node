/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_MSG_H
#define SMF_SML_MSG_H

#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/buffer.h>

#include <cstdint>

namespace smf {
	namespace sml {

		/**
		 * list of messages ready to serialize (see to_sml() function)
		 */
		using messages_t = std::vector < cyng::tuple_t >;

		/**
		 * Convert a list of messages into a SML message
		 */
		cyng::buffer_t to_sml(sml::messages_t const&);

		/**
		 * Add SML trailer and tail
		 */
		cyng::buffer_t boxing(std::vector<cyng::buffer_t> const&);

	}
}
#endif
