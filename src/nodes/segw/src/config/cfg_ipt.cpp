/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_ipt.h>

#include <smf/obis/defs.h>

 
#ifdef _DEBUG_SEGW
#include <iostream>
#endif


namespace smf {

	cfg_ipt::cfg_ipt(cfg& c)
		: cfg_(c)
	{}

	namespace {
		std::string enabled_path() {
			return cyng::to_path(cfg::sep, cfg_ipt::root, "enabled");
		}
	}

	bool cfg_ipt::is_enabled() const {
		return cfg_.get_value(enabled_path(), false);
	}

	const std::string cfg_ipt::root = cyng::to_str(OBIS_ROOT_IPT_PARAM);

}