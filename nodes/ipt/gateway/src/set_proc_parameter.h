/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_GW_SET_PROC_PARAMETER_H
#define NODE_GW_SET_PROC_PARAMETER_H


#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis.h>

#include <cyng/log.h>
#include <cyng/vm/controller.h>
#include <cyng/store/db.h>

namespace node
{
	namespace sml
	{
		//
		//	forward declaration
		//
		class res_generator;
		class cfg_ipt;

		/**
		 * 
		 */
		class set_proc_parameter
		{
		public:
			set_proc_parameter(cyng::logging::log_ptr
				, res_generator& sml_gen
				, cyng::store::db& config_db
				, cfg_ipt&);

			void generate_response(obis_path_t const&
				, std::string trx
				, cyng::buffer_t srv_id
				, std::string user
				, std::string pwd
				, cyng::param_t	param);

		private:
			void _81490d0700ff(obis_path_t::const_iterator
				, obis_path_t::const_iterator
				, std::string trx
				, cyng::buffer_t srv_id
				, std::string user
				, std::string pwd
				, cyng::param_t	param);

			//void _81490d070002(obis_path_t::const_iterator
			//	, obis_path_t::const_iterator
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
			cyng::store::db& config_db_;

			/**
			 * IP-T configuration
			 */
			cfg_ipt& ipt_;

		};
	}	//	sml
}

#endif
