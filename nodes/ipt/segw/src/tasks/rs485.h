/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_RS485_H
#define NODE_SEGW_TASK_RS485_H

#include "../cfg_mbus.h"
#include <cyng/log.h>
#include <cyng/async/mux.h>

namespace node
{
	/**
	 * manage RS 485 interface
	 */
	class cache;
	class rs485
	{
	public:
		//	[0] write entry
		using msg_0 = std::tuple<>;

		using signatures_t = std::tuple<msg_0>;

	public:
		rs485(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cache&
			, std::size_t);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - write an entry
		 *
		 */
		cyng::continuation process();

	private:
		void search();
		void readout();

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
		cfg_mbus cfg_;

		/**
		 *  LMN task
		 */
		std::size_t const tsk_;

		enum class state {
			SEARCH,
			READOUT,
		} state_;
	};

}

#endif