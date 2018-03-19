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
		using msg_0 = std::tuple<>;
		using signatures_t = std::tuple<msg_0>;

	public:
		watchdog(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::store::db&
			, cyng::object obj
			, std::chrono::seconds monitor);
		void run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * sucessful cluster login
		 */
		cyng::continuation process();

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