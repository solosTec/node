/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SML_CFG_PUSH_H
#define NODE_SML_CFG_PUSH_H


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
		 * Managing push operations
		 */
		class cfg_push
		{
		public:
			cfg_push(cyng::logging::log_ptr
				, res_generator& sml_gen
				, cyng::controller& vm
				, cyng::store::db& config_db);

		private:
			void sml_set_proc_push_source(cyng::context& ctx);
			void sml_set_proc_push_interval(cyng::context& ctx);
			void sml_set_proc_push_delay(cyng::context& ctx);
			void sml_set_proc_push_name(cyng::context& ctx);
			void sml_set_proc_push_profile(cyng::context& ctx);
			void sml_set_proc_push_service(cyng::context& ctx);
			void sml_set_proc_push_delete(cyng::context& ctx);
			void sml_set_proc_push_reserve(cyng::context& ctx);

			/**
			 * @return true if new record was inserted successfully
			 */
			bool create_push_op(cyng::store::table* tbl
				, cyng::table::key_type
				, cyng::table::data_type
				, boost::uuids::uuid tag);


		private:
			cyng::logging::log_ptr logger_;


			/**
			 * buffer for current SML message
			 */
			res_generator& sml_gen_;

			/**
			 * configuration db
			 */
			cyng::store::db& config_db_;

		};

	}	//	sml
}

#endif
