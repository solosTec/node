/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <router.h>

namespace smf {

	router::router(cyng::controller& ctl, cfg& config)
		: ctl_(ctl)
		, cfg_(config)
	{}
}