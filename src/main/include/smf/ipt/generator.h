/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_GENERATOR_H
#define NODE_IPT_GENERATOR_H

#include <NODE_project_info.h>
#include <smf/ipt/config.h>

namespace node
{
	namespace ipt
	{
		namespace gen
		{
			cyng::vector_t ipt_req_login_public(master_config_t const& config, std::size_t idx);
			cyng::vector_t ipt_req_login_scrambled(master_config_t const& config, std::size_t idx);
		}
	}
}

#endif
