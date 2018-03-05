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
			, cluster_config_t const& cfg_cls
			, boost::asio::ip::tcp::endpoint
			, std::string const& doc_root
			, uint16_t watchdog
			, int timeout);
		void run();
		void stop();

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

		void create_cache();
		void db_insert(cyng::context& ctx);
		void db_modify_by_attr(cyng::context& ctx);

		void ws_read(cyng::context& ctx);

		void snyc_table(std::string const&);

		void update_sys_cpu_usage_total(std::string const&, boost::uuids::uuid);
		void update_sys_mem_virtual_total(std::string const&, boost::uuids::uuid);
		void update_sys_mem_virtual_used(std::string const&, boost::uuids::uuid);

		void subscribe_devices(std::string const&, boost::uuids::uuid);
		void subscribe_sessions(std::string const&, boost::uuids::uuid);

	private:
		cyng::async::base_task& base_;
		bus::shared_type bus_;
		cyng::logging::log_ptr logger_;
		const cluster_config_t	config_;

		/**
		 * global data cache
		 */
		cyng::store::db cache_;
		http::server	server_;
		std::size_t master_;

	};
	
}

#endif