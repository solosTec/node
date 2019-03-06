/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_TSDB_DISPATCHER_H
#define NODE_TSDB_DISPATCHER_H

#include <cyng/log.h>
#include <cyng/table/body.hpp>
#include <cyng/vm/controller.h>
#include <cyng/async/mux.h>

namespace node 
{

	class dispatcher
	{
	public:
		dispatcher(cyng::async::mux&, cyng::logging::log_ptr, std::set<std::size_t>);

		/**
		 * register provided functions
		 */
		void register_this(cyng::controller&);

		void res_subscribe(std::string const&		//	[0] table name
			, cyng::table::key_type		//	[1] table key
			, cyng::table::data_type	//	[2] record
			, std::uint64_t				//	[3] generation
			, boost::uuids::uuid		//	[4] origin session id
			, std::size_t tsk);

	private:
		void db_res_insert(cyng::context& ctx);
		void db_res_remove(cyng::context& ctx);
		void db_res_modify_by_attr(cyng::context& ctx);
		void db_res_modify_by_param(cyng::context& ctx);
		void db_req_insert(cyng::context& ctx);
		void db_req_remove(cyng::context& ctx);
		void db_req_modify_by_param(cyng::context& ctx);

		void distribute(std::string const&		//	[0] table name
			, cyng::table::key_type		//	[1] table key
			, cyng::table::data_type	//	[2] record
			, std::uint64_t	gen
			, boost::uuids::uuid origin);


	private:
		cyng::async::mux& mux_;
		cyng::logging::log_ptr logger_;
		std::set<std::size_t> const tasks_;
	};


}

#endif
