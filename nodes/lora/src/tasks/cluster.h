/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LORA_TASK_CLUSTER_H
#define NODE_LORA_TASK_CLUSTER_H

#include "../processor.h"
#include "../dispatcher.h"
#include "../sync_db.h"
#include <smf/http/srv/auth.h>

#include <smf/cluster/bus.h>
#include <smf/cluster/config.h>
#include <smf/https/srv/server.h>

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
			, boost::asio::ssl::context& ctx
			, boost::uuids::uuid tag
			, bool keep_xml_files
			, cluster_config_t const& cfg
			, boost::asio::ip::tcp::endpoint ep
			, std::size_t timeout
			, std::string const& doc_root
			, auth_dirs const& ad
			, std::set<boost::asio::ip::address> const&
			, std::map<std::string, std::string> const& redirects);
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

		void sync_table(std::string const& name);

	private:
		cyng::async::base_task& base_;
		bus::shared_type bus_;
		cyng::logging::log_ptr logger_;
		cluster_redundancy const config_;

		/**
		 * the HTTPS server
		 */
		https::server	server_;

		/**
		 * global data cache
		 */
		cyng::store::db cache_;
		processor processor_;

		dispatcher	dispatcher_;
		db_sync	db_sync_;
	};
	
}

#endif
