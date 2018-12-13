/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IP_MASTER_TASK_WATCHDOG_H
#define NODE_IP_MASTER_TASK_WATCHDOG_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>

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
			, cyng::controller& vm
			, std::uint16_t watchdog
			, std::string account);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * activity
		 */
		cyng::continuation process();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		cyng::controller& vm_;
		const std::chrono::seconds watchdog_;
		std::chrono::system_clock::time_point last_activity_;
	};
	
}

#endif