/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_GET_PROC_PARAMETER_H
#define NODE_SEGW_GET_PROC_PARAMETER_H


#include <smf/sml/intrinsics/obis.h>

#include <cyng/log.h>

namespace node
{
	//
	//	forward declaration
	//
	namespace ipt
	{
		class config_ipt;
	}
	class cache;
	class storage;
	namespace sml
	{
		/**
		 * forward declaration
		 */
		class res_generator;
		class config_sensor_params;
		class config_data_collector;
		class get_proc_parameter
		{
		public:
			get_proc_parameter(cyng::logging::log_ptr
				, res_generator& sml_gen
				, cache& c
				, storage& db
				, node::ipt::config_ipt&
				, config_sensor_params&
				, config_data_collector&);

			void generate_response(obis code
				, std::string trx
				, cyng::buffer_t srv_id
				, std::string user
				, std::string pwd);

		private:
			void class_op_log_status_word(std::string trx, cyng::buffer_t srv_id);
			void code_root_device_ident(std::string trx, cyng::buffer_t srv_id);
			void code_root_device_time(std::string trx, cyng::buffer_t srv_id);
			void code_root_ntp(std::string trx, cyng::buffer_t srv_id);
			void code_root_access_rights(std::string trx, cyng::buffer_t srv_id);
			void code_root_custom_interface(std::string trx, cyng::buffer_t srv_id);
			void code_root_custom_param(std::string trx, cyng::buffer_t srv_id);
			void code_root_wan(std::string trx, cyng::buffer_t srv_id);
			void code_root_gsm(std::string trx, cyng::buffer_t srv_id);
			void code_root_gprs_param(std::string trx, cyng::buffer_t srv_id);
			void code_root_ipt_state(std::string trx, cyng::buffer_t srv_id);
			void code_root_w_mbus_status(std::string trx, cyng::buffer_t srv_id);
			void code_if_wmbus(std::string trx, cyng::buffer_t srv_id);
			void code_root_lan_dsl(std::string trx, cyng::buffer_t srv_id);
			void code_if_lan_dsl(std::string trx, cyng::buffer_t srv_id);
			void code_root_memory_usage(std::string trx, cyng::buffer_t srv_id);
			void code_root_active_devices(std::string trx, cyng::buffer_t srv_id);
			void code_root_visible_devices(std::string trx, cyng::buffer_t srv_id);
			void code_root_device_info(std::string trx, cyng::buffer_t srv_id);
			void code_if_1107(std::string trx, cyng::buffer_t srv_id);
			void storage_time_shift(std::string trx, cyng::buffer_t srv_id);
			void push_operations(std::string trx, cyng::buffer_t srv_id);
			void list_services(std::string trx, cyng::buffer_t srv_id);
			void actuators(std::string trx, cyng::buffer_t srv_id);
			void code_if_edl(std::string trx, cyng::buffer_t srv_id);
			void class_mbus(std::string trx, cyng::buffer_t srv_id);

		private:
			cyng::logging::log_ptr logger_;

			/**
			 * buffer for current SML message
			 */
			res_generator& sml_gen_;

			/**
			 * configuration db
			 */
			cache& cache_;

			/**
			 * SQL database
			 */
			storage& storage_;

			node::ipt::config_ipt&	config_ipt_;
			config_sensor_params& config_sensor_params_;
			config_data_collector& config_data_collector_;

		};
	}	//	sml
}

#endif
