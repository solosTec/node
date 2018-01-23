/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#ifndef NODE_SETUP_TASK_STORAGE_DB_H
#define NODE_SETUP_TASK_STORAGE_DB_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/db/session_pool.h>
#include <cyng/table/meta_interface.h>
#include <cyng/store/db.h>
#include <map>

namespace node
{
	class storage_db
	{
	public:
		using msg_0 = std::tuple<std::string, std::size_t>;
		using msg_1 = std::tuple<std::string, cyng::vector_t, cyng::vector_t, std::uint64_t>;
		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		storage_db(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::store::db& cache
			, cyng::param_map_t);
		void run();
		void stop();

		/**
		 * slot [0] - load cache
		 */
		cyng::continuation process(std::string name, std::size_t sync_tsk);

		/**
		 * slot [1] - insert data
		 */
		cyng::continuation process(std::string name
			, cyng::vector_t const&
			, cyng::vector_t const&
			, std::uint64_t);

		/**
		 * static method to create tables.
		 */
		static int init_db(cyng::tuple_t);
		static std::map<std::string, cyng::table::meta_table_ptr> init_meta_map();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::db::session_pool pool_;
		std::map<std::string, cyng::table::meta_table_ptr>	meta_map_;

		/**
		 * global data cache
		 */
		cyng::store::db& cache_;

	};
	
}

#endif