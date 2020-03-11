/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_LIMITER_H
#define NODE_SEGW_TASK_LIMITER_H

#include <cyng/log.h>
#include <cyng/async/mux.h>

namespace node
{
	/**
	 * Monitors the size of all data collector tables. If the specified
	 * maximum size is exeeded the oldest entries are deleted.
	 *
	 * All required logic is implemented in bridge::shrink()
	 */
	class bridge;
	class limiter
	{
	public:
		//	[0] write entry
		using msg_0 = std::tuple<>;

		using signatures_t = std::tuple<msg_0>;

	public:
		limiter(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, bridge&
			, std::chrono::hours);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - write an entry
		 *
		 */
		cyng::continuation process();

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * permanent storage
		 */
		bridge& bridge_;

		std::chrono::hours interval_time_;
	};

}

#endif