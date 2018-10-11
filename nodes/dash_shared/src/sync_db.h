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

	void res_subscribe(cyng::logging::log_ptr
		, cyng::store::db&
		, std::string const&		//	[0] table name
		, cyng::table::key_type		//	[1] table key
		, cyng::table::data_type	//	[2] record
		, std::uint64_t				//	[3] generation
		, boost::uuids::uuid		//	[4] origin session id
		, std::size_t tsk);

	void db_res_insert(cyng::logging::log_ptr
		, cyng::store::db&
		, std::string const&		//	[0] table name
		, cyng::table::key_type		//	[1] table key
		, cyng::table::data_type	//	[2] record
		, std::uint64_t	gen
		, boost::uuids::uuid origin);

	void db_req_insert(cyng::logging::log_ptr
		, cyng::store::db&
		, std::string const&		//	[0] table name
		, cyng::table::key_type		//	[1] table key
		, cyng::table::data_type	//	[2] record
		, std::uint64_t	gen
		, boost::uuids::uuid origin);

	void db_req_remove(cyng::logging::log_ptr
		, cyng::store::db&
		, std::string const&		//	[0] table name
		, cyng::table::key_type		//	[1] table key
		, boost::uuids::uuid origin);

	void db_res_remove(cyng::logging::log_ptr
		, cyng::store::db&
		, std::string const&		//	[0] table name
		, cyng::table::key_type		//	[1] table key
		, boost::uuids::uuid origin);
}

#endif
