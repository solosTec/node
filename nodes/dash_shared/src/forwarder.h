/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_HTTP_FORWARDER_H
#define NODE_HTTP_FORWARDER_H

#include <smf/http/srv/cm_interface.h>
#include <cyng/log.h>
#include <cyng/vm/context.h>
#include <cyng/dom/reader.h>
#include <cyng/vm/controller.h>
#include <cyng/store/db.h>
#include <pugixml.hpp>
#include <boost/uuid/random_generator.hpp>

namespace node 
{
	void fwd_insert(cyng::logging::log_ptr
		, cyng::context& ctx
		, cyng::reader<cyng::object> const&
		, boost::uuids::uuid);

	void fwd_delete(cyng::logging::log_ptr
		, cyng::context& ctx
		, cyng::reader<cyng::object> const&);

	void fwd_modify(cyng::logging::log_ptr
		, cyng::context& ctx
		, cyng::reader<cyng::object> const&);

	/**
	 * Stop a running session
	 */
	void fwd_stop(cyng::logging::log_ptr
		, cyng::context& ctx
		, cyng::reader<cyng::object> const&);

	/**
	 * Communicate to IP-T proxy
	 */
	void fwd_config_gateway(cyng::logging::log_ptr
		, cyng::context& ctx
		, boost::uuids::uuid tag_ws
		, std::string const& channel
		, cyng::reader<cyng::object> const&);

	/**
	 * reboot gateway
	 */
	//void fwd_reboot(cyng::logging::log_ptr
	//	, cyng::context& ctx
	//	, boost::uuids::uuid tag_ws
	//	, cyng::reader<cyng::object> const&);

	/**
	 * Send a process parameter request to an gateway
	 */
	//void fwd_query_gateway(cyng::logging::log_ptr
	//	, cyng::context& ctx
	//	, boost::uuids::uuid tag_ws
	//	, cyng::reader<cyng::object> const&);

	class forward
	{
	public:
		forward(cyng::logging::log_ptr
			, cyng::store::db& db
			, connection_manager_interface&);

		/**
		 * register provided functions
		 */
		void register_this(cyng::controller&);

	private:

		void cfg_upload_devices(cyng::context& ctx);
		void cfg_upload_gateways(cyng::context& ctx);
		void cfg_upload_meter(cyng::context& ctx);

		void read_device_configuration_3_2(cyng::context& ctx, pugi::xml_document const& doc, bool);
		void read_device_configuration_4_0(cyng::context& ctx, pugi::xml_document const& doc, bool);
		void read_device_configuration_5_x(cyng::context& ctx, pugi::xml_document const& doc, bool);

		void cfg_download_devices(cyng::context& ctx);
		void cfg_download_gateways(cyng::context& ctx);
		void cfg_download_meters(cyng::context& ctx);
		void cfg_download_messages(cyng::context& ctx);
		void cfg_download_LoRa(cyng::context& ctx);

		void trigger_download(boost::uuids::uuid tag, std::string table, std::string filename);

	private:
		cyng::logging::log_ptr logger_;
		cyng::store::db& db_;
		connection_manager_interface & connection_manager_;
		boost::uuids::random_generator uidgen_;
	};
}

#endif
