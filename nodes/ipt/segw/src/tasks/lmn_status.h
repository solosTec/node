/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_LMN_STATUS_H
#define NODE_SEGW_TASK_LMN_STATUS_H

#include <smf/sml/status.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>

namespace node
{
	/**
	 * Control status bit for wireless M-Bus or
	 */
	class cache;
	class lmn_status
	{
	public:
		//	[0] receive data
		using msg_0 = std::tuple<std::size_t, std::size_t>;
		//	[1] status (open/closed)
		using msg_1 = std::tuple<bool>;

		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		lmn_status(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cache& cfg
			, sml::status_bits);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - receive data
		 *
		 */
		cyng::continuation process(std::size_t bytes, std::size_t msg);

		/**
		 * @brief slot [1] - status (open/closed)
		 *
		 */
		cyng::continuation process(bool);


	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * configuration management
		 */
		cache& cache_;

		/**
		 * status bit to control
		 */
		sml::status_bits const status_bits_;

	};

}

#endif