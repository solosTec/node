/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_SET_PROC_PARAMETER_H
#define NODE_SEGW_SET_PROC_PARAMETER_H


#include <smf/sml/intrinsics/obis.h>

#include <cyng/log.h>
#include <cyng/intrinsics/sets.h>

namespace node
{
	namespace ipt
	{
		class config_ipt;
	}
	class cache;
	namespace sml
	{
		/**
		 * forward declaration
		 */
		class config_sensor_params;
		class config_data_collector;
		class config_security;
		class config_access;
		class res_generator;
		class set_proc_parameter
		{
		public:
			set_proc_parameter(cyng::logging::log_ptr
				, res_generator& sml_gen
				, cache& cfg
				, node::ipt::config_ipt&
				, config_sensor_params&
				, config_data_collector&
				, config_security&
				, config_access&);

			void generate_response(obis_path const&
				, std::string trx
				, cyng::buffer_t srv_id
				, std::string user
				, std::string pwd
				, cyng::param_t	param);

		private:
			void _81490d0700ff(obis_path::const_iterator
				, obis_path::const_iterator
				, std::string trx
				, cyng::buffer_t srv_id
				, std::string user
				, std::string pwd
				, cyng::param_t	param);

			void storage_time_shift(obis_path::const_iterator
				, obis_path::const_iterator
				, std::string trx
				, cyng::buffer_t srv_id
				, std::string user
				, std::string pwd
				, cyng::param_t	param);

			//void _81490d070002(obis_path::const_iterator
			//	, obis_path::const_iterator
			//	, std::string trx
			//	, cyng::buffer_t srv_id
			//	, std::string user
			//	, std::string pwd
			//	, cyng::param_t	param);
			//void sml_set_proc_activate(cyng::context& ctx);
			//void sml_set_proc_deactivate(cyng::context& ctx);
			//void sml_set_proc_delete(cyng::context& ctx);

			//void sml_set_proc_if1107_param(cyng::context& ctx);
			//void sml_set_proc_if1107_device(cyng::context& ctx);

			//void sml_set_proc_mbus_param(cyng::context& ctx);
			//void sml_set_proc_sensor(cyng::context& ctx);
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

			node::ipt::config_ipt&	config_ipt_;
			config_sensor_params& config_sensor_params_;
			config_data_collector& config_data_collector_;
			config_security& config_security_;
			config_access& config_access_;

		};
	}	//	sml
}

#endif
