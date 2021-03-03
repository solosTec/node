/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_BUS_TRANSPILER_H
#define SMF_IPT_BUS_TRANSPILER_H

#include <smf/ipt/header.h>
#include <smf/ipt/codes.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/intrinsics/container.h>

namespace smf
{
	namespace ipt	
	{
		cyng::deque_t gen_instructions(header const&, cyng::buffer_t&&);
	}	//	ipt
}


#endif
