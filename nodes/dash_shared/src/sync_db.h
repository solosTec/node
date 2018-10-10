/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_HTTP_SYNC_DB_H
#define NODE_HTTP_SYNC_DB_H

#include <cyng/log.h>
#include <cyng/store/db.h>

namespace node 
{
	void create_cache(cyng::logging::log_ptr, cyng::store::db&);
	void clear_cache(cyng::store::db&, boost::uuids::uuid);
}

#endif
