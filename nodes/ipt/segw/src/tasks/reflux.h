/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_REFLUX_H
#define NODE_SEGW_TASK_REFLUX_H

#include <cyng/log.h>
#include <cyng/async/mux.h>

namespace node
{
	/**
	 * redirect data back from serial port
	 */
	class reflux
	{
	public:
		//	[0] receive data
		using msg_0 = std::tuple<cyng::buffer_t, std::size_t>;

		using signatures_t = std::tuple<msg_0>;

	public:
		reflux(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, std::function<void(cyng::buffer_t, std::size_t)>);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - receive data
		 *
		 */
		cyng::continuation process(cyng::buffer_t, std::size_t);

	private:
		//void init();


	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		std::function<void(cyng::buffer_t, std::size_t)> cb_;
		/**
		 * manage task state
		 */
		//bool initialized_;
	};

}

#endif