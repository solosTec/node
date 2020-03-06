/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_GET_LIST_H
#define NODE_SEGW_GET_LIST_H


#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis.h>

#include <cyng/log.h>
#include <cyng/vm/controller.h>
#include <cyng/store/db.h>

namespace node
{
	class cache;
	class storage;
	namespace sml
	{
		//
		//	forward declaration
		//
		class res_generator;

		/**
		 * 
		 */
		class get_list
		{
		public:
			get_list(cyng::logging::log_ptr
				, res_generator& sml_gen
				, cache& cfg
				, storage& db);

			void generate_response(obis&& code
				, std::string		//	[0] trx
				, cyng::buffer_t	//	[1] client id
				, cyng::buffer_t	//	[2] server id <- meter/sensor ID with the requested data
				, std::string		//	[3] reqFileId
				, std::string		//	[4] user
				, std::string);		//	[5] password

		private:
			void current_data_record(std::string trx
				, cyng::buffer_t client_id
				, cyng::buffer_t srv_id
				, std::string req_field);

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

			/**
			 * SQL database
			 */
			storage& storage_;

		};
	}	//	sml
}

#endif
