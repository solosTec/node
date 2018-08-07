/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_CSV_TASK_CLOCK_H
#define NODE_CSV_TASK_CLOCK_H

//#include <smf/cluster/bus.h>
//#include <smf/cluster/config.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/version.h>

namespace node
{

	class clock
	{
	public:
		using msg_0 = std::tuple<cyng::version>;
		using msg_1 = std::tuple<>;
		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		clock(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::size_t tsk_db
			, std::chrono::minutes offset);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * sucessful cluster login
		 */
		cyng::continuation process(cyng::version const&);

		/**
		 * @brief slot [1]
		 *
		 * reconnect
		 */
		cyng::continuation process();

	private:
		std::chrono::system_clock::time_point calculate_trigger_tp();
		void generate_last_period();
		void generate_current_period();

	private:
		cyng::async::base_task& base_;
		cyng::logging::log_ptr logger_;
		const std::size_t tsk_db_;
		const std::chrono::minutes offset_;

		/**
		 * task state
		 */
		enum {
			TASK_STATE_INITIAL_,
			TASK_STATE_RUNNING_,
		} state_;

		std::chrono::system_clock::time_point next_trigger_tp_;

	};	
}

#endif