/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_ROUTER_H
#define SMF_SEGW_ROUTER_H

#include <cfg.h>

#include <cyng/task/task_fwd.h>
#include <cyng/vm/vm_fwd.h>

 namespace smf {

	 /**
	  * routing of IP-T/SML messages
	  */
	 class router
	 {
	 public:
		 router(cyng::controller&, cfg& config);

	 private:
		 cyng::controller& ctl_;
		 cfg& cfg_;
	 };
}

#endif