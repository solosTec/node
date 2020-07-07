/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_CLOCK_H
#define NODE_SEGW_TASK_CLOCK_H

#include <cyng/log.h>
#include <cyng/async/mux.h>

namespace node
{
	/**
	 * cyclic update of "time-offset"
	 */
	class cache;
	class clock
	{
	public:
		//	[0] write entry
		using msg_0 = std::tuple<>;

		using signatures_t = std::tuple<msg_0>;

	public:
		clock(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cache&
			, std::chrono::minutes);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - write an entry
		 *
		 */
		cyng::continuation process();

	private:
		void init();


	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * permanent storage
		 */
		cache& cache_;

		/**
		 * interval time (2 minutes)
		 */
		std::chrono::minutes const interval_time_;

		/**
		 * manage task state
		 */
		bool initialized_;
	};

}

#endif