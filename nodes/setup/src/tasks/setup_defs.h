/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SETUP_DEFINITIONS_H
#define NODE_SETUP_DEFINITIONS_H

#include <cstdint>

namespace node
{
	enum table_state : std::uint32_t
	{
		TS_INITIAL = 0,
		TS_SYNC = 1,
		TS_READY = 2
	};
	
}

#endif