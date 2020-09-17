/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_CONFIG_BROKER_H
#define NODE_SEGW_CONFIG_BROKER_H


#include <smf/sml/intrinsics/obis.h>
#include "../cfg_broker.h"

#include <cyng/log.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/store/store_fwd.h>

namespace node
{
	//
	//	forward declaration
	//
	class cache;
	namespace sml
	{

		/**
		 * manage broker parameters
		 */
		class res_generator;
		class config_broker
		{
		public:
			config_broker(cyng::logging::log_ptr
				, res_generator& sml_gen
				, cache& c);

			void get_proc_params(std::string trx, cyng::buffer_t srv_id) const;
			void get_proc_params_port(std::string trx, cyng::buffer_t srv_id) const;

			void set_proc_params(obis_path_t const& path, cyng::buffer_t srv_id, cyng::object);


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
			cfg_broker cfg_;

		};
	}	//	sml
}

#endif
