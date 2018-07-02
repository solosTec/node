/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_TASK_STORAGE_DB_H
#define NODE_IPT_STORE_TASK_STORAGE_DB_H

#include "../processors/db_processor.h"
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/db/session_pool.h>
#include <cyng/table/meta_interface.h>
#include <cyng/vm/context.h>
#include <map>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	class storage_db
	{
	public:
		using msg_0 = std::tuple<std::uint32_t, std::uint32_t, std::string, std::string, cyng::buffer_t>;
		using signatures_t = std::tuple<msg_0>;

	public:
		storage_db(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::param_map_t);
		void run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * push data
		 */
		cyng::continuation process(std::uint32_t channel
			, std::uint32_t source
			, std::string const& target
			, std::string const& protocol
			, cyng::buffer_t const& data);

		/**
		 * static method to create tables.
		 */
		static int init_db(cyng::tuple_t);
		static cyng::table::mt_table init_meta_map(std::string const&);

	private:
		void stop_writer(cyng::context& ctx);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::db::session_pool pool_;
		const std::string schema_;
		cyng::table::mt_table	meta_map_;
		std::map<std::uint64_t, db_processor>	lines_;
		boost::uuids::random_generator rng_;
		std::list<std::uint64_t>	hit_list_;
	};
}

#endif