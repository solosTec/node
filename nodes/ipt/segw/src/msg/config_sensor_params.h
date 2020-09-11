/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_CONFIG_SENSOR_PARAMS_H
#define NODE_SEGW_CONFIG_SENSOR_PARAMS_H


#include <smf/sml/intrinsics/obis.h>

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
		 * manage sensor parameters configuration
		 */
		class res_generator;
		class config_sensor_params
		{
		public:
			config_sensor_params(cyng::logging::log_ptr
				, res_generator& sml_gen
				, cache& c);

			void get_proc_params(std::string trx, cyng::buffer_t srv_id) const;
			void set_param(obis_path_t const& path
				, cyng::buffer_t srv_id
				, cyng::param_t const& param);

		private:
			void set_bitmask(cyng::store::table* tbl
				, cyng::buffer_t srv_id
				, cyng::object);

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

		};
	}	//	sml
}

#endif
