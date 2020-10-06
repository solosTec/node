/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_IEC_H
#define NODE_SEGW_TASK_IEC_H

//#include "../cfg_mbus.h"
#include "../decoder.h"

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller_fwd.h>

namespace node
{
	class cache;
	/**
	 * Manage RS 485 interface as IEC protocol.
	 */
	class iec
	{
	public:
		//	[0] write entry
		using msg_0 = std::tuple<>;

		using signatures_t = std::tuple<msg_0>;

	public:
		iec(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::controller&
			, cache&
			, std::size_t);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - ack
		 *
		 */
		cyng::continuation process();


	private:

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * config database
		 */
		cache& cache_;
		//cfg_mbus cfg_;

		/**
		 *  LMN task
		 */
		std::size_t const tsk_;

	};


}

#endif