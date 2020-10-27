/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_GET_PROFILE_LIST_H
#define NODE_SEGW_GET_PROFILE_LIST_H


#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis.h>
#include <smf/sml/protocol/message.h>

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
		class get_profile_list
		{
		public:
			get_profile_list(cyng::logging::log_ptr
				, res_generator& sml_gen
				, cache& cfg
				, storage& db);

			messages_t generate_response(obis code
				, std::string		//	[0] trx
				, cyng::buffer_t	//	[1] client id
				, cyng::buffer_t	//	[2] server id
				, std::string		//	[3] user
				, std::string		//	[4] password
				, std::chrono::system_clock::time_point		//	[5] start time
				, std::chrono::system_clock::time_point);

		private:
			messages_t class_op_log(std::string trx
				, cyng::buffer_t client_id
				, cyng::buffer_t srv_id
				, std::chrono::system_clock::time_point start
				, std::chrono::system_clock::time_point end);

#ifdef _DEBUG
			//	demo
			messages_t profile_15_minute(std::string trx
				, cyng::buffer_t client_id
				, cyng::buffer_t srv_id
				, std::chrono::system_clock::time_point start
				, std::chrono::system_clock::time_point end);
#endif

			messages_t get_profile(obis code
				, std::string trx
				, cyng::buffer_t client_id
				, cyng::buffer_t srv_id
				, std::chrono::system_clock::time_point start
				, std::chrono::system_clock::time_point end);

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
