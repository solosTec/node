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
	void init(cyng::logging::log_ptr logger
		, cyng::store::db&
		, boost::uuids::uuid tag
		, boost::asio::ip::tcp::endpoint
		, std::chrono::seconds connection_open_timeout
		, std::chrono::seconds connection_close_timeout
		, bool connection_auto_login
		, bool connection_auto_enabled
		, bool connection_superseed);

	void insert_msg(cyng::store::db&
		, cyng::logging::severity
		, std::string const&
		, boost::uuids::uuid tag);

	void insert_msg(cyng::store::table* tbl
		, cyng::logging::severity
		, std::string const&
		, boost::uuids::uuid tag);

	cyng::table::record connection_lookup(cyng::store::table* tbl, cyng::table::key_type&& key);
	bool connection_erase(cyng::store::table* tbl, cyng::table::key_type&& key, boost::uuids::uuid tag);
}

#endif

