/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#ifndef NODE_SML_KERNEL_H
#define NODE_SML_KERNEL_H


#include <smf/sml/defs.h>
#include <smf/sml/status.h>
#include "sml_reader.h"
#include <smf/sml/protocol/generator.h>
#include <cyng/log.h>
#include <cyng/vm/controller.h>
#include <cyng/store/db.h>

namespace node
{
	namespace sml
	{
		/**
		* Provide core functions of an SML gateway
		*/
		class kernel
		{
		public:
			kernel(cyng::logging::log_ptr
				, cyng::controller&
				, status&
				, cyng::store::db& config_db
				, bool
				, std::string account
				, std::string pwd
				, std::string manufacturer
				, std::string model
				, cyng::mac48 mac);

			/**
			* reset kernel
			*/
			void reset();

		private:
			//void append_msg(cyng::tuple_t&&);

			void sml_msg(cyng::context& ctx);
			void sml_eom(cyng::context& ctx);

			void sml_public_open_request(cyng::context& ctx);
			void sml_public_close_request(cyng::context& ctx);
			void sml_get_proc_parameter_request(cyng::context& ctx);
			void sml_get_proc_status_word(cyng::context& ctx);
			void sml_get_proc_device_id(cyng::context& ctx);
			void sml_get_proc_mem_usage(cyng::context& ctx);
			void sml_get_proc_lan_if(cyng::context& ctx);
			void sml_get_proc_lan_config(cyng::context& ctx);
			void sml_get_proc_ntp_config(cyng::context& ctx);
			void sml_get_proc_device_time(cyng::context& ctx);
			void sml_get_proc_active_devices(cyng::context& ctx);
			void sml_get_proc_visible_devices(cyng::context& ctx);
			void sml_get_proc_device_info(cyng::context& ctx);
			void sml_get_proc_ipt_state(cyng::context& ctx);
			void sml_get_proc_ipt_param(cyng::context& ctx);
			void sml_get_proc_sensor_property(cyng::context& ctx);
			void sml_get_proc_data_collector(cyng::context& ctx);
			void sml_get_proc_1107_if(cyng::context& ctx);
			void sml_get_proc_0080800000FF(cyng::context& ctx);
			void sml_get_proc_push_ops(cyng::context& ctx);

		public:
			/**
			* Global status word
			*/
			status&	status_word_;

		private:
			cyng::logging::log_ptr logger_;

			/**
			* configuration db
			*/
			cyng::store::db& config_db_;

			const bool server_mode_;
			const std::string account_;
			const std::string pwd_;

			//
			//	hardware
			//
			const std::string manufacturer_;
			const std::string model_;
			const cyng::buffer_t server_id_;

			/**
			* read SML tree and generate commands
			*/
			sml_reader reader_;

			/**
			* buffer for current SML message
			*/
			res_generator sml_gen_;

		};

	}	//	sml
}

#endif