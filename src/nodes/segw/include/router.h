/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_ROUTER_H
#define SMF_SEGW_ROUTER_H

#include <cfg.h>
#include <smf/ipt/bus.h>

#include <cyng/task/task_fwd.h>
#include <cyng/vm/vm_fwd.h>

 namespace smf {

	 /**
	  * routing of IP-T/SML messages
	  */
	 class router
	 {
	 public:
		 router(cyng::controller&, cfg& config, cyng::logger);
		 void start();

	 private:
		 void ipt_cmd(ipt::header const&, cyng::buffer_t&&);
		 void ipt_stream(cyng::buffer_t&&);
		 void auth_state(bool);

	 private:
		 cyng::controller& ctl_;
		 cfg& cfg_;
		 cyng::logger logger_;
		 std::unique_ptr<ipt::bus>	bus_;
	 };
}

#endif