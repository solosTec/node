/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_CSV_TASK_PROFILE_24_H_H
#define NODE_CSV_TASK_PROFILE_24_H_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/policy.h>
#include <cyng/intrinsics/version.h>

namespace node
{

	class profile_24_h
	{
	public:
		using msg_0 = std::tuple<>;
		using signatures_t = std::tuple<msg_0>;

	public:
		profile_24_h(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::size_t tsk_db
			, std::chrono::minutes offset
			, std::chrono::minutes frame
			, std::string format);
		cyng::continuation run();
		void stop();

		/**
		 * @brief slot [0]
		 *
		 * unused
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
		const std::chrono::minutes frame_;
		const std::string format_;

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