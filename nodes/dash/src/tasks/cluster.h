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
			, cluster_config_t const& cfg_cls
			, boost::asio::ip::tcp::endpoint
			, std::string const& doc_root);
		cyng::continuation run();
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
		void http_upload_start(cyng::context& ctx);
		void http_upload_data(cyng::context& ctx);
		void http_upload_var(cyng::context& ctx);
		void http_upload_progress(cyng::context& ctx);
		void http_upload_complete(cyng::context& ctx);
		//void http_moved(cyng::context& ctx);

		void cfg_download_devices(cyng::context& ctx);
		void cfg_download_gateways(cyng::context& ctx);
		void cfg_download_messages(cyng::context& ctx);
		void cfg_download_LoRa(cyng::context& ctx);

		void cfg_upload_devices(cyng::context& ctx);
		void cfg_upload_gateways(cyng::context& ctx);
		void cfg_upload_meter(cyng::context& ctx);

		void res_query_srv_visible(cyng::context& ctx);
		void res_query_srv_active(cyng::context& ctx);
		void res_query_firmware(cyng::context& ctx);

		void sync_table(std::string const&);

		void update_sys_cpu_usage_total(std::string const&, http::websocket_session* wss);
		void update_sys_cpu_count(std::string const&, http::websocket_session* wss);
		void update_sys_mem_virtual_total(std::string const&, http::websocket_session* wss);
		void update_sys_mem_virtual_used(std::string const&, http::websocket_session* wss);

		void subscribe(std::string table, std::string const&, boost::uuids::uuid);
		void subscribe_table_device_count(std::string const&, boost::uuids::uuid);
		void subscribe_table_gateway_count(std::string const&, boost::uuids::uuid);
		void subscribe_table_meter_count(std::string const&, boost::uuids::uuid);
		void subscribe_table_session_count(std::string const&, boost::uuids::uuid);
		void subscribe_table_target_count(std::string const&, boost::uuids::uuid);
		void subscribe_table_connection_count(std::string const&, boost::uuids::uuid);
		void subscribe_table_msg_count(std::string const&, boost::uuids::uuid);
		void subscribe_table_LoRa_count(std::string const&, boost::uuids::uuid);

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

        void start_sys_task();
        void stop_sys_task();

		void read_device_configuration_3_2(cyng::context& ctx, pugi::xml_document const& doc);
		void read_device_configuration_4_0(cyng::context& ctx, pugi::xml_document const& doc);
		void read_device_configuration_5_x(cyng::context& ctx, pugi::xml_document const& doc);

		void display_loading_icon(boost::uuids::uuid tag, bool, std::string const&);
		void display_loading_level(boost::uuids::uuid tag, std::size_t, std::string const&);

		void trigger_download(boost::uuids::uuid tag, std::string table, std::string filename);

	private:
		cyng::async::base_task& base_;
		boost::uuids::random_generator rgn_;

		/**
		 * communication bus to master
		 */
		bus::shared_type bus_;
		cyng::logging::log_ptr logger_;
		const cluster_redundancy config_;

		/**
		 * global data cache
		 */
		cyng::store::db cache_;

		/**
		 * the HTTP server
		 */
		http::server	server_;

		/**
		 * system task
		 */
        std::size_t sys_tsk_;

		/**
		 * form values
		 */
		std::map<boost::uuids::uuid, std::map<std::string, std::string>>	form_data_;
	};
	
}

#endif
