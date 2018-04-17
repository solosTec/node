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
#include <boost/uuid/random_generator.hpp>

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
		void subscribe_cache();
		void res_subscribe(cyng::context& ctx);
		void db_res_insert(cyng::context& ctx);
		void db_res_remove(cyng::context& ctx);
		void db_res_modify_by_attr(cyng::context& ctx);
		void db_res_modify_by_param(cyng::context& ctx);
		void db_req_insert(cyng::context& ctx);
		void db_req_remove(cyng::context& ctx);
		void db_req_modify_by_param(cyng::context& ctx);

		void ws_read(cyng::context& ctx);

		void sync_table(std::string const&);

		void update_sys_cpu_usage_total(std::string const&, http::websocket_session* wss);
		void update_sys_cpu_count(std::string const&, http::websocket_session* wss);
		void update_sys_mem_virtual_total(std::string const&, http::websocket_session* wss);
		void update_sys_mem_virtual_used(std::string const&, http::websocket_session* wss);

		void subscribe_devices(std::string const&, boost::uuids::uuid);
		void subscribe_gateways(std::string const&, boost::uuids::uuid);
		void subscribe_sessions(std::string const&, boost::uuids::uuid);
		void subscribe_targets(std::string const&, boost::uuids::uuid);
		void subscribe_connections(std::string const&, boost::uuids::uuid);
		void subscribe_cluster(std::string const&, boost::uuids::uuid);
		void subscribe_table_device_count(std::string const&, boost::uuids::uuid);
		void subscribe_table_gateway_count(std::string const&, boost::uuids::uuid);
		void subscribe_table_meter_count(std::string const&, boost::uuids::uuid);
		void subscribe_table_session_count(std::string const&, boost::uuids::uuid);
		void subscribe_table_target_count(std::string const&, boost::uuids::uuid);
		void subscribe_table_connection_count(std::string const&, boost::uuids::uuid);
		void subscribe_monitor_msg(std::string const&, boost::uuids::uuid);

		void update_channel(std::string const&, std::size_t);

		void sig_ins(cyng::store::table const*
			, cyng::table::key_type const&
			, cyng::table::data_type const&
			, std::uint64_t
			, boost::uuids::uuid);
		void sig_del(cyng::store::table const*, cyng::table::key_type const&, boost::uuids::uuid);
		void sig_clr(cyng::store::table const*, boost::uuids::uuid);
		void sig_mod(cyng::store::table const*
			, cyng::table::key_type const&
			, cyng::attr_t const&
			, std::uint64_t
			, boost::uuids::uuid);

	private:
		cyng::async::base_task& base_;
		boost::uuids::random_generator rgn_;

		/**
		 * communication bus to master
		 */
		bus::shared_type bus_;
		cyng::logging::log_ptr logger_;
		const cluster_config_t	config_;

		/**
		 * global data cache
		 */
		cyng::store::db cache_;

		/**
		 * the HTTP server
		 */
		http::server	server_;

		/**
		 * current master node
		 */
		std::size_t master_;

	};
	
}

#endif