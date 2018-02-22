/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MASTER_DB_H
#define NODE_MASTER_DB_H

#include <cyng/log.h>
#include <cyng/store/db.h>
#include <boost/uuid/uuid.hpp>

namespace node 
{
	void init(cyng::logging::log_ptr logger, cyng::store::db&, boost::uuids::uuid tag);
}

#endif

