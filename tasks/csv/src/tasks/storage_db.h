/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_CSV_TASK_STORAGE_DB_H
#define NODE_CSV_TASK_STORAGE_DB_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/db/session_pool.h>
#include <cyng/table/meta_interface.h>

namespace node
{
	class storage_db
	{
	public:
		using msg_0 = std::tuple<std::chrono::system_clock::time_point
			, std::chrono::system_clock::time_point
			, std::chrono::minutes>;
		using signatures_t = std::tuple<msg_0>;

	public:
		storage_db(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::param_map_t
			, cyng::param_map_t);
		cyng::continuation run();
		void stop();

		/**
		 * slot [0] - generate CSV file
		 */
		cyng::continuation process(std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end
			, std::chrono::minutes interval);


		/**
		 * static method to create tables.
		 */
		static cyng::table::mt_table init_meta_map(std::string const&);

	private:
		void generate_csv_files(std::chrono::system_clock::time_point start
			, std::chrono::system_clock::time_point end
			, std::chrono::minutes interval);


	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::db::session_pool pool_;
		const cyng::param_map_t cfg_db_;
		const cyng::param_map_t cfg_csv_;

		const std::string schema_;
		cyng::table::mt_table	meta_map_;

		/**
		 * task state
		 */
		enum {
			TASK_STATE_WAITING_,
			TASK_STATE_OPEN_,
		} state_;

	};
	
}

#endif