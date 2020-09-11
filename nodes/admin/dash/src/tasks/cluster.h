/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_DASH_TASK_CLUSTER_H
#define NODE_DASH_TASK_CLUSTER_H

#include <smf/cluster/bus.h>
#include <smf/cluster/config.h>
#include <smf/http/srv/server.h>
#include "../../../shared/src/dispatcher.h"
#include "../../../shared/src/sync_db.h"
#include "../../../shared/src/forwarder.h"
#include "../../../shared/src/form_data.h"
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/store/db.h>
#include <boost/uuid/random_generator.hpp>
#include <pugixml.hpp>

namespace node
{
	/**
	 * 15.859 kB
	 * 15.792 kB
	 */
	class cluster
	{
	public:
		using msg_0 = std::tuple<cyng::version>;
		using msg_1 = std::tuple<>;
		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		cluster(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, boost::uuids::uuid cluster_tag
			, cluster_config_t const& cfg_cls
			, boost::asio::ip::tcp::endpoint
			, std::size_t timeout
			, std::uint64_t max_upload_size
			, std::string const& doc_root
			, std::string const& nickname
#ifdef NODE_SSL_INSTALLED
			, auth_dirs const& ad
#endif
			, std::set<boost::asio::ip::address> const&
			, std::map<std::string, std::string> const& redirects_specific
			, std::map<std::string, std::string> const& redirects_generic
			, bool https_rewrite
		);
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

		//void res_subscribe(cyng::context& ctx);

		/**
		 * This function receives all incoming data from
		 * the web app.
		 */
		void ws_read(cyng::context& ctx);

		/**
		 * subscribe a table
		 */
		void sync_table(std::string const&);

        void start_sys_task();
        void stop_sys_task();

	private:
		cyng::async::base_task& base_;
		boost::uuids::random_generator uidgen_;

		/**
		 * communication bus to master
		 */
		bus::shared_type bus_;
		cyng::logging::log_ptr logger_;
		cluster_redundancy const config_;

		/**
		 * global data cache
		 */
		cyng::store::db cache_;

		/**
		 * the HTTP server
		 */
		http::server	server_;

		/**
		 * data dispatcher
		 */
		dispatcher dispatcher_;

		/**
		 * data synchronizer
		 */
		db_sync db_sync_;

		/**
		 * data forward data changes to SMF master
		 */
		forward forward_;

		/**
		 * handle form data
		 */
		form_data form_data_;

		/**
		 * system task
		 */
        std::size_t sys_tsk_;

	};
	
}

#endif
