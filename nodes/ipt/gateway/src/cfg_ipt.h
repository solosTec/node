/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SML_CFG_IPT_H
#define NODE_SML_CFG_IPT_H


#include <smf/sml/defs.h>
#include <smf/mbus/header.h>
#include <smf/sml/protocol/generator.h>

#include <cyng/log.h>
#include <cyng/vm/controller.h>
#include <cyng/store/db.h>

namespace node
{
	namespace sml
	{
		/**
		 * Managing IP-T parameters
		 */
		class cfg_ipt
		{
		public:
			cfg_ipt(cyng::logging::log_ptr
				, res_generator& sml_gen
				, cyng::controller& vm
				, cyng::store::db& config_db
				, node::ipt::redundancy const& cfg);

			void get_proc_ipt_params(cyng::object trx, cyng::object server_id);

		private:
			void sml_set_proc_ipt_param_address(cyng::context& ctx);
			void sml_set_proc_ipt_param_port_target(cyng::context& ctx);
			void sml_set_proc_ipt_param_user(cyng::context& ctx);
			void sml_set_proc_ipt_param_pwd(cyng::context& ctx);

		private:
			cyng::logging::log_ptr logger_;

			/**
			 * IP-T configuration
			 */
			node::ipt::redundancy cfg_ipt_;

			/**
			 * buffer for current SML message
			 */
			res_generator& sml_gen_;

			/**
			 * configuration db
			 */
			cyng::store::db& config_db_;

		};

	}	//	sml
}

#endif
