/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_CONFIG_IEC_H
#define NODE_SEGW_CONFIG_IEC_H


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
		 * manage security parameters
		 */
		class res_generator;
		class config_iec
		{
		public:
			config_iec(cyng::logging::log_ptr
				, res_generator& sml_gen
				, cache& c);

			CYNG_ATTR_NODISCARD cyng::tuple_t get_proc_params(std::string trx, cyng::buffer_t srv_id) const;
			//void set_param(node::sml::obis code
			//	, cyng::buffer_t srv_id
			//	, cyng::param_t const& param);

			void set_meter(std::uint8_t nr, cyng::param_map_t&& map);
			void set_param(obis const& code, cyng::param_t param);

		private:

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
