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
#include "../../../dash_shared/src/dispatcher.h"
#include "../../../dash_shared/src/sync_db.h"
#include "../../../dash_shared/src/forwarder.h"
#include "../../../dash_shared/src/form_data.h"
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
			, std::string const& doc_root
			, std::set<boost::asio::ip::address> const&);
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

		void res_subscribe(cyng::context& ctx);

		void ws_read(cyng::context& ctx);

		void cfg_download_devices(cyng::context& ctx);
		void cfg_download_gateways(cyng::context& ctx);
		void cfg_download_messages(cyng::context& ctx);
		void cfg_download_LoRa(cyng::context& ctx);

		//void cfg_upload_devices(cyng::context& ctx);
		//void cfg_upload_gateways(cyng::context& ctx);
		//void cfg_upload_meter(cyng::context& ctx);

		void sync_table(std::string const&);

		void update_sys_cpu_usage_total(std::string const&, http::websocket_session* wss);
		void update_sys_cpu_count(std::string const&, http::websocket_session* wss);
		void update_sys_mem_virtual_total(std::string const&, http::websocket_session* wss);
		void update_sys_mem_virtual_used(std::string const&, http::websocket_session* wss);

        void start_sys_task();
        void stop_sys_task();

		//void read_device_configuration_3_2(cyng::context& ctx, pugi::xml_document const& doc);
		//void read_device_configuration_4_0(cyng::context& ctx, pugi::xml_document const& doc);
		//void read_device_configuration_5_x(cyng::context& ctx, pugi::xml_document const& doc);

		void trigger_download(boost::uuids::uuid tag, std::string table, std::string filename);

	private:
		cyng::async::base_task& base_;
		boost::uuids::random_generator uidgen_;

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
