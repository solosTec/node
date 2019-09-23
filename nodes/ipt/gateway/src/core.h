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
#include <smf/ipt/config.h>
#include <smf/sml/protocol/generator.h>
#include "data.h"
#include "attention.h"
#include "cfg_ipt.h"
#include "cfg_push.h"
#include "get_proc_parameter.h"
#include "set_proc_parameter.h"
#include "get_profile_list.h"

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
				, cyng::store::db& config_db
				, node::ipt::redundancy const& cfg
				, bool
				, std::string account
				, std::string pwd
				, bool accept_all);

			/**
			 * reset kernel
			 */
			void reset();

		private:
			void sml_msg(cyng::context& ctx);
			void sml_eom(cyng::context& ctx);

			void sml_public_open_request(cyng::context& ctx);
			void sml_public_close_request(cyng::context& ctx);
			void sml_public_close_response(cyng::context& ctx);
			void sml_get_proc_parameter_request(cyng::context& ctx);
			void sml_set_proc_parameter_request(cyng::context& ctx);
			void sml_get_profile_list_request(cyng::context& ctx);

		private:
			cyng::logging::log_ptr logger_;

			/**
			 * configuration db
			 */
			cyng::store::db& config_db_;

			bool const server_mode_;
			std::string const account_;
			std::string const pwd_;

			//
			//	configuration
			//
			bool const accept_all_;

			/**
			 * buffer for current SML message
			 */
			res_generator sml_gen_;

			/**
			 * process incoming data
			 */
			data data_;
			cfg_ipt ipt_;
			cfg_push push_;
			attention attention_;
			get_proc_parameter get_proc_parameter_;
			set_proc_parameter set_proc_parameter_;
			get_profile_list get_profile_list_;
		};

	}	//	sml
}

#endif
