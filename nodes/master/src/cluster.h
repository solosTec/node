/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MASTER_CLUSTER_H
#define NODE_MASTER_CLUSTER_H

#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/store/db.h>
#include <cyng/vm/context.h>
#include <boost/uuid/random_generator.hpp>

namespace node 
{
	class session;
	class cluster
	{

	public:
		cluster(cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cyng::store::db&
			, std::atomic<std::uint64_t>& global_configuration);

		cluster(cluster const&) = delete;
		cluster& operator=(cluster const&) = delete;


		/**
		 * register VM callbacks
		 */
		void register_this(cyng::context& ctx);

	private:
		void bus_req_com_sml(cyng::context& ctx);
		void bus_res_com_sml(cyng::context& ctx);

		void bus_res_attention_code(cyng::context& ctx);

		void bus_req_com_task(cyng::context& ctx);
		void bus_req_com_node(cyng::context& ctx);

		std::tuple<session const*, cyng::table::record, cyng::buffer_t, boost::uuids::uuid> find_peer(cyng::table::key_type const& key_session
			, const cyng::store::table* tbl_session
			, const cyng::store::table* tbl_gw);

	private:
		cyng::async::mux& mux_;
		cyng::logging::log_ptr logger_;
		cyng::store::db& db_;
		std::atomic<std::uint64_t>& global_configuration_;

		/**
		 * generate meter tags
		 */
		boost::uuids::random_generator uidgen_;
	};

}



#endif

