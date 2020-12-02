/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IEC_62056_TASK_CLUSTER_H
#define NODE_IEC_62056_TASK_CLUSTER_H

#include <smf/cluster/bus.h>
#include <smf/cluster/config.h>
#include "../sync_db.h"
#include "../server.h"

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/store/db.h>

namespace node
{

	class cluster
	{
	public:
		using msg_0 = std::tuple<cyng::version>;
		using msg_1 = std::tuple<>;
		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		cluster(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, boost::uuids::uuid tag
			, cluster_config_t const& cfg
			, boost::asio::ip::tcp::endpoint ep
			, bool client_login
			, bool verbose
			, std::string const& target);
		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0]
		 *
		 * sucessful cluster login
		 */
		cyng::continuation process(cyng::version const&);

		/**
		 * @brief slot [1]
		 *
		 * reconnect
		 */
		cyng::continuation process();

	private:
		void connect();
		void reconfigure(cyng::context& ctx);
		void reconfigure_impl();
		void session_callback(boost::uuids::uuid, cyng::vector_t&&);

		/**
		 * subscribe a table
		 */
		void sync_table(std::string const&);

	private:
		/**
		 * communication bus to master
		 */
		cyng::async::base_task& base_;
		bus::shared_type bus_;
		cyng::logging::log_ptr logger_;
		cluster_redundancy const config_;

		bool const client_login_;
		bool const verbose_;	//	parser
		std::string const target_;	//	push target name

		/**
		 * global data cache
		 */
		cyng::store::db cache_;

		/**
		 * data synchronizer
		 */
		db_sync db_sync_;

		/**
		 *  TCP/IP server
		 */
		server server_;

		boost::uuids::random_generator uuid_gen_;

	};
	
}

#endif
