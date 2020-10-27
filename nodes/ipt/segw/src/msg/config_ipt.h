/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_CONFIG_IPT_H
#define NODE_SEGW_CONFIG_IPT_H


#include <smf/sml/intrinsics/obis.h>

#include <cyng/log.h>
#include <cyng/intrinsics/sets.h>

namespace node
{
	//
	//	forward declaration
	//
	namespace sml {
		class res_generator;
	}
	class cache;
	namespace ipt
	{
		using node::sml::obis_path_t;

		/**
		 * manage IP-T configuration
		 */
		class config_ipt
		{
		public:
			config_ipt(cyng::logging::log_ptr
				, node::sml::res_generator& sml_gen
				, cache& c);

			CYNG_ATTR_NODISCARD cyng::tuple_t get_proc_params(std::string trx, cyng::buffer_t srv_id) const;
			void set_param(obis_path_t const& path, cyng::param_t const& param);

		private:
			cyng::logging::log_ptr logger_;


			/**
			 * buffer for current SML message
			 */
			node::sml::res_generator& sml_gen_;

			/**
			 * configuration db
			 */
			cache& cache_;

		};
	}	//	sml
}

#endif
