/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_OBISLOG_H
#define NODE_SEGW_TASK_OBISLOG_H

#include <cyng/log.h>
#include <cyng/async/mux.h>

namespace node
{
	/**
	 * write cyclic OBISLOG entries
	 */
	class bridge;
	class obislog
	{
	public:
		//	[0] write entry
		using msg_0 = std::tuple<>;


		using signatures_t = std::tuple<msg_0>;

	public:
		obislog(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, bridge&
			, std::chrono::minutes);

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

		std::chrono::minutes interval_time_;
	};

}

#endif