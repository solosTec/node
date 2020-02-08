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

//
//	macro to create OBIS codes without the 0x prefix
//

#define OBIS_CODE(p1, p2, p3, p4, p5, p6)	\
	node::sml::make_obis (0x##p1, 0x##p2, 0x##p3, 0x##p4, 0x##p5, 0x##p6)

#endif
