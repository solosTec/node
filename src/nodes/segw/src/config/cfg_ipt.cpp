/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <config/cfg_ipt.h>

 
#ifdef _DEBUG_SEGW
#include <iostream>
#endif


namespace smf {

	cfg_ipt::cfg_ipt(cfg& c)
		: cfg_(c)
	{}

}