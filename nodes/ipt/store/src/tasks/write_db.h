/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_TASK_WRITE_DB_H
#define NODE_IPT_STORE_TASK_WRITE_DB_H

#include <smf/sml/protocol/parser.h>
#include <smf/sml/exporter/db_exporter.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/buffer.h>
#include <cyng/vm/controller.h>
#include <cyng/db/session.h>

namespace node
{
	class storage_db;
	class write_db
	{
		friend storage_db;

	public:
		using msg_0 = std::tuple<cyng::buffer_t>;
		using msg_1 = std::tuple<>;
		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		write_db(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, boost::uuids::uuid tag
			, std::uint32_t channel
			, std::uint32_t source
			, std::string target
			, cyng::db::session);
		void run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * push data
		 */
		cyng::continuation process(cyng::buffer_t const&);

		/**
		 * @brief slot [1]
		 *
		 * stop 
		 */
		cyng::continuation process();

	private:
		void sml_msg(cyng::context& ctx);
		void sml_eom(cyng::context& ctx);

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::controller vm_;
		sml::parser parser_;
		cyng::db::session db_;
		sml::db_exporter exporter_;	
	};
}

#endif