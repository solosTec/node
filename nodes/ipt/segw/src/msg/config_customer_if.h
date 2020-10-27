/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_CONFIG_CUSTOMER_IF_H
#define NODE_SEGW_CONFIG_CUSTOMER_IF_H


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
		class config_customer_if
		{
		public:
			config_customer_if(cyng::logging::log_ptr
				, res_generator& sml_gen
				, cache& c);

			CYNG_ATTR_NODISCARD cyng::tuple_t get_proc_params(std::string trx, cyng::buffer_t srv_id) const;
			CYNG_ATTR_NODISCARD cyng::tuple_t get_ip_address(std::string trx, cyng::buffer_t srv_id) const;
			CYNG_ATTR_NODISCARD cyng::tuple_t get_nms(std::string trx, cyng::buffer_t srv_id) const;

			void set_params(obis_path_t const& path
				, cyng::buffer_t srv_id
				, cyng::object const& obj);


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
