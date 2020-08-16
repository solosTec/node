/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IEC_SYNC_DB_H
#define NODE_IEC_SYNC_DB_H

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
		struct tbl_descr {
			std::string const name_;
			bool const custom_;
			inline tbl_descr(std::string name, bool custom)
				: name_(name)
				, custom_(custom)
			{}
		};


		/**
		 * List of all required tables
		 */
		const static std::array<tbl_descr, 2>	tables_;
	public:
		db_sync(cyng::logging::log_ptr, cyng::store::db&);

		/**
		 * register provided functions
		 */
		void register_this(cyng::controller&);

	private:
		void res_subscribe(cyng::context& ctx);

		void db_res_insert(cyng::context& ctx);
		void db_res_remove(cyng::context& ctx);
		void db_res_modify_by_attr(cyng::context& ctx);
		void db_res_modify_by_param(cyng::context& ctx);

		/**
		 * Cannot use the implementation of the store_domain (CYNG) because
		 * of the additional source (UUID) parameter.
		 */
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
}

#endif
