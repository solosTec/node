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

			void get_proc_ipt_params(std::string trx, cyng::buffer_t server_id) const;

			/**
			 * overwrite the specified parameter
			 */
			void set_param(obis, cyng::param_t const&);

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

			//
			//	fixed values
			//
			std::uint8_t wait_time_;	//	minutes
			std::uint32_t repetitions_;	//	counter
			bool ssl_;

		};

	}	//	sml
}

#endif
