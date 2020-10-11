/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_SEGW_ROUTER_H
#define NODE_IPT_SEGW_ROUTER_H

#include "msg/config_ipt.h"
#include "msg/config_sensor_params.h"
#include "msg/config_data_collector.h"
#include "msg/config_security.h"
#include "msg/config_access.h"
#include "msg/config_iec.h"
#include "msg/config_broker.h"
#include "msg/config_customer_if.h"
#include "msg/get_proc_parameter.h"
#include "msg/set_proc_parameter.h"
#include "msg/get_profile_list.h"
#include "msg/get_list.h"
#include "msg/attention.h"

#include <smf/sml/protocol/generator.h>

#include <cyng/store/db.h>
#include <cyng/log.h>
#include <cyng/vm/controller_fwd.h>

namespace node
{
	/**
	 * Central message dispatcher 
	 */
	class storage;
	class cache;
	class router
	{

	public:
		/**
		 * @param tsk_net network task id. This is the task that manage all communication 
		 * regarding the IP-T master.
		 */
		router(cyng::logging::log_ptr
			, bool const server_mode
			, cyng::controller& vm
			, cache&
			, storage& db
			, std::string const& account
			, std::string const& pwd
			, bool accept_all);

		virtual ~router();

	private:
		void sml_msg(cyng::context& ctx);

		/**
		 * End Of Message
		 */
		void sml_eom(cyng::context& ctx);

		void sml_public_open_request(cyng::context& ctx);
		void sml_public_close_request(cyng::context& ctx);
		void sml_public_close_response(cyng::context& ctx);
		void sml_get_proc_parameter_request(cyng::context& ctx);
		void sml_set_proc_parameter_request(cyng::context& ctx);
		void sml_get_profile_list_request(cyng::context& ctx);
		void sml_get_list_request(cyng::context& ctx);

	private:
		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * Working in client (as session) or server (customer interface)
		 */
		bool const server_mode_;

		/**
		 * global data cache
		 */
		cache& cache_;

		/**
		 * buffer for current SML message
		 */
		sml::res_generator sml_gen_;

		std::string const account_;
		std::string const pwd_;

		/**
		 * Accept all incoming request and don't check server ID or 
		 * credentials.
		 */
		bool const accept_all_;

		ipt::config_ipt	config_ipt_;
		sml::config_sensor_params config_sensor_params_;
		sml::config_data_collector config_data_collector_;
		sml::config_security config_security_;
		sml::config_access config_access_;
		sml::config_iec config_iec_;
		sml::config_broker config_broker_;
		sml::config_customer_if config_customer_if_;

		sml::get_proc_parameter get_proc_parameter_;
		sml::set_proc_parameter set_proc_parameter_;
		sml::get_profile_list get_profile_list_;
		sml::get_list get_list_;
		sml::attention attention_;

	};


}
#endif
