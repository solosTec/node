/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_DISTRIBUTOR_H
#define SMF_SEGW_DISTRIBUTOR_H

#include <config/cfg_lmn.h>

#include <cyng/task/task_fwd.h>

 namespace smf {

	 /**
	  * distribute configuration changes in the system
	  */
	 class distributor
	 {
	 public:
		 distributor(cyng::controller&, cfg& config);

		 void update(std::string key, cyng::object const&);

	 private:
		 void update_lmn(std::string const& key, lmn_type, cyng::object const& val);

	 private:
		 cyng::controller& ctl_;
		 cfg& cfg_;
	 };
}

#endif