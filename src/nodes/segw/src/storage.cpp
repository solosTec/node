/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <storage.h>

namespace smf {

	storage::storage(cyng::db::session db)
		: db_(db)
	{}

}