/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_STORAGE_H
#define SMF_SEGW_STORAGE_H

#include <storage_functions.h>

#include <cyng/task/task_fwd.h>

 namespace smf {

	 /**
	  * manage SQL tables
	  */
	 class storage
	 {
	 public:
		 storage(cyng::db::session);

		 bool cfg_insert(cyng::object const&, cyng::object const&);
		 bool cfg_update(cyng::object const&, cyng::object const&);
		 bool cfg_remove(cyng::object const&);

	 private:
		 cyng::db::session db_;
	 };
}

#endif