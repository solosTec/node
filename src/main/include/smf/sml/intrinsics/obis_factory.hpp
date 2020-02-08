/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SML_INTRINSICS_OBIS_FACTORY_HPP
#define NODE_SML_INTRINSICS_OBIS_FACTORY_HPP

#include <smf/sml/intrinsics/obis.h>
#include <cyng/intrinsics/sets.h>

namespace node
{
	namespace sml
	{
		/**
		 * Generate an OBIS code
		 */
		obis make_obis(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);

		cyng::tuple_t tuple_from_path(obis_path path);

	}	//	sml
}	//	node

#endif
