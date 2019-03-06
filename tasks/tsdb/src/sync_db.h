/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_TSDB_SYNC_DB_H
#define NODE_TSDB_SYNC_DB_H

#include <cyng/log.h>
#include <cyng/store/db.h>
#include <cyng/vm/controller.h>

namespace node 
{
	void create_cache(cyng::logging::log_ptr, cyng::store::db&);
	void clear_cache(cyng::store::db&, boost::uuids::uuid);

	class db_sync
	{
	public:
		db_sync(cyng::logging::log_ptr, cyng::store::db&);

		/**
		 * register provided functions
		 */
		void register_this(cyng::controller&);

	private:
		void db_res_insert(cyng::context& ctx);
		void db_res_remove(cyng::context& ctx);
		void db_res_modify_by_attr(cyng::context& ctx);
		void db_res_modify_by_param(cyng::context& ctx);
		void db_req_insert(cyng::context& ctx);
		void db_req_remove(cyng::context& ctx);
		void db_req_modify_by_param(cyng::context& ctx);

		void db_insert(cyng::context& ctx
			, std::string const&		//	[0] table name
			, cyng::table::key_type		//	[1] table key
			, cyng::table::data_type	//	[2] record
			, std::uint64_t	gen
			, boost::uuids::uuid origin);

	private:
		cyng::logging::log_ptr logger_;
		cyng::store::db& db_;
	};

	void res_subscribe(cyng::logging::log_ptr
		, cyng::store::db&
		, std::string const&		//	[0] table name
		, cyng::table::key_type		//	[1] table key
		, cyng::table::data_type	//	[2] record
		, std::uint64_t				//	[3] generation
		, boost::uuids::uuid		//	[4] origin session id
		, std::size_t tsk);

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

	void db_res_modify_by_attr(cyng::logging::log_ptr
		, cyng::store::db&
		, std::string const&		//	[0] table name
		, cyng::table::key_type		//	[1] table key
		, cyng::attr_t				//	[2] attribute
		, boost::uuids::uuid origin);

	void db_res_modify_by_param(cyng::logging::log_ptr
		, cyng::store::db&
		, std::string const&		//	[0] table name
		, cyng::table::key_type		//	[1] table key
		, cyng::param_t				//	[2] parameter
		, boost::uuids::uuid origin);

	void db_req_modify_by_param(cyng::logging::log_ptr
		, cyng::store::db&
		, std::string const&		//	[0] table name
		, cyng::table::key_type	//	[1] table key
		, cyng::param_t			//	[2] parameter
		, std::uint64_t			//	[3] generation
		, boost::uuids::uuid);
}

#endif
