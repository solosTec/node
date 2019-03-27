/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_MASTER_TASK_WATCHDOG_H
#define NODE_MASTER_TASK_WATCHDOG_H

#include <cyng/log.h>
#include <cyng/async/task/base_task.h>
#include <cyng/async/mux.h>
#include <cyng/store/db.h>


namespace node
{

	class watchdog
	{
	public:
		using msg_0 = std::tuple<boost::uuids::uuid>;
		using signatures_t = std::tuple<msg_0>;

	public:
		watchdog(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::store::db&
			, cyng::object obj
			, std::chrono::seconds monitor);
		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0]
		 *
		 * master shutdown
		 */
		cyng::continuation process(boost::uuids::uuid tag);

	private:
		void send_watchdogs();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::object session_obj_;
		std::chrono::seconds monitor_;
	};
	
}

#endif