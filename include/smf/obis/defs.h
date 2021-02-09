/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_OBIS_DEFS_H
#define SMF_OBIS_DEFS_H

#include <cyng/obj/intrinsics/obis.h>

 //
 //	macro to create OBIS codes without the 0x prefix
 //
#define DEFINE_OBIS_CODE(p1, p2, p3, p4, p5, p6, name)	\
	OBIS_##name (0x##p1, 0x##p2, 0x##p3, 0x##p4, 0x##p5, 0x##p6)

#define DEFINE_OBIS(p1, p2, p3, p4, p5, p6)	\
	cyng::obis(0x##p1, 0x##p2, 0x##p3, 0x##p4, 0x##p5, 0x##p6)

//
//	define two constexpr at once. First is a variable of type obis and the name starts with "OBIS_".
//	second is an unsigned integer with the same value and the name starts with "CODE_".
//
#define OBIS_CODE_DEFINITION(p1, p2, p3, p4, p5, p6, name)	\
	constexpr static cyng::obis OBIS_##name { 0x##p1, 0x##p2, 0x##p3, 0x##p4, 0x##p5, 0x##p6 }; \
	constexpr static std::uint64_t CODE_##name { 0x##p1##p2##p3##p4##p5##p6 };


namespace smf {

#include "defs.ipp"

}

#endif
