/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_STORAGE_H
#define SMF_SEGW_STORAGE_H

#include <storage_functions.h>

 namespace smf {

	 /**
	  * manage SQL tables
	  */
	 class storage
	 {
	 public:
		 storage(cyng::db::session);

	 private:
		 cyng::db::session db_;
	 };
}

#endif