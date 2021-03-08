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
//#include <smf/ipt/scramble_key.h>

#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/intrinsics/container.h>

#include <tuple>

namespace smf
{
	namespace ipt	
	{
		cyng::deque_t gen_instructions(header const&, cyng::buffer_t&&);

		std::tuple<response_t, std::uint16_t, std::string> ctrl_res_login(cyng::buffer_t&& data);

	}	//	ipt
}


#endif
