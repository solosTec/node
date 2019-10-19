/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_TASK_IEC_DB_CONSUMER_H
#define NODE_IPT_STORE_TASK_IEC_DB_CONSUMER_H

#include <smf/sml/exporter/db_iec_exporter.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/db/session_pool.h>
#include <cyng/table/meta_interface.h>
#include <cyng/vm/context.h>
#include <unordered_map>

namespace node
{
	class iec_db_consumer
	{
	public:
		using msg_0 = std::tuple<std::uint64_t, std::string>;
		using msg_1 = std::tuple<std::uint64_t, boost::uuids::uuid, cyng::buffer_t, std::size_t, cyng::param_map_t >;
		using msg_2 = std::tuple<std::uint64_t, boost::uuids::uuid, std::string, std::string, bool, std::size_t>;
		using signatures_t = std::tuple<msg_0, msg_1, msg_2>;

	public:
		iec_db_consumer(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::size_t ntid	//	network task id
			, cyng::param_map_t);
		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0]
		 *
		 * create a new line
		 */
		cyng::continuation process(std::uint64_t line, std::string);

		/**
		 * @brief slot [1]
		 *
		 * receive push data
		 */
		cyng::continuation process(std::uint64_t
			, boost::uuids::uuid
			, cyng::buffer_t const&
			, std::size_t
			, cyng::param_map_t
		);

		/**
		 * @brief slot [2]
		 *
		 * close line
		 */
		cyng::continuation process(std::uint64_t line
			, boost::uuids::uuid
			, std::string meter
			, std::string status
			, bool bcc
			, std::size_t);

		/**
		 * static method to create tables.
		 */
		static int init_db(cyng::tuple_t);
		static cyng::table::meta_map_t init_meta_map(std::string const&);

	private:
		void register_consumer();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		const std::size_t ntid_;
		cyng::db::session_pool pool_;
		const std::string schema_;
		const std::chrono::seconds period_;
		const bool ignore_null_;	//!< don't store values equal to null
		std::size_t total_count_, skipped_count_;
		cyng::table::meta_map_t	meta_map_;
		enum task_state {
			TASK_STATE_INITIAL,
			TASK_STATE_DB_OK,
			TASK_STATE_REGISTERED,
		} task_state_;
		std::unordered_map<std::uint64_t, iec::db_exporter>	lines_;
	};
}

#endif
