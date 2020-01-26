/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_PUSH_H
#define NODE_SEGW_TASK_PUSH_H

#include <smf/sml/intrinsics/obis.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>

namespace node
{
	/**
	 * write cyclic OBISLOG entries
	 */
	class cache;
	class push
	{
	public:
		//	[0] write entry
		using msg_0 = std::tuple<>;


		using signatures_t = std::tuple<msg_0>;

	public:
		push(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cache& cfg
			, cyng::buffer_t srv_id
			, cyng::buffer_t profile
			, std::chrono::seconds interval
			, std::chrono::seconds delay
			, std::string target);

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
		cache& cache_;

		cyng::buffer_t const srv_id_;
		sml::obis const profile_;
		std::chrono::seconds interval_;
		std::chrono::seconds delay_;
		std::string target_;
	};

}

#endif