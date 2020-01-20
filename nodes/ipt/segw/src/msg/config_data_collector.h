/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_CONFIG_DATA_COLLECTOR_H
#define NODE_SEGW_CONFIG_DATA_COLLECTOR_H


#include <smf/sml/intrinsics/obis.h>

#include <cyng/log.h>
#include <cyng/intrinsics/sets.h>
//#include <cyng/store/store_fwd.h>
#include <cyng/store/table.h>

namespace node
{
	//
	//	forward declaration
	//
	class cache;
	namespace sml
	{

		/**
		 * manage data mirror
		 */
		class res_generator;
		class config_data_collector
		{
		public:
			config_data_collector(cyng::logging::log_ptr
				, res_generator& sml_gen
				, cache& c);

			void get_proc_params(std::string trx, cyng::buffer_t srv_id) const;
			void get_push_operations(std::string trx, cyng::buffer_t srv_id) const;

			void set_param(cyng::buffer_t srv_id
				, std::uint8_t nr
				, cyng::param_map_t&& params);

			void set_push_operations(cyng::buffer_t srv_id
				, std::string user
				, std::string pwd
				, std::uint8_t nr
				, cyng::param_map_t&& params);

			void clear_data_collector(cyng::buffer_t srv_id
				, cyng::param_map_t&& params);

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

		/**
		 * @return true if specified meter has any entries
		 */
		void insert_data_collector(cyng::store::table*
			, cyng::table::key_type const&
			, cyng::param_map_t const& params
			, cyng::store::table*
			, boost::uuids::uuid tag);

		void update_data_collector(cyng::store::table*
			, cyng::table::key_type const&
			, cyng::param_map_t const& params
			, cyng::store::table*
			, boost::uuids::uuid tag);

		void update_data_mirror(cyng::store::table*
			, cyng::object srv_id
			, cyng::object nr
			, cyng::param_map_t const& params
			, boost::uuids::uuid tag);

		void insert_push_ops(cyng::store::table*
			, cyng::table::key_type const&
			, cyng::param_map_t const& params
			, boost::uuids::uuid tag);

		void update_push_ops(cyng::store::table*
			, cyng::table::key_type const&
			, cyng::param_map_t const& params
			, boost::uuids::uuid tag);

		void loop_data_mirror(cyng::store::table const*
			, cyng::tuple_t& msg
			, cyng::buffer_t srv_id
			, std::uint8_t nr);


	}	//	sml
}

#endif