/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_TASK_GPIO_H
#define NODE_SEGW_TASK_GPIO_H

#include <cyng/log.h>
#include <cyng/async/mux.h>

#include <cyng/compatibility/file_system.hpp>

namespace node
{
	/**
	 * control GPIO LED
	 */
	class gpio
	{
	public:
		//	[0] static switch ON/OFF
		using msg_0 = std::tuple<bool>;

		//	[1] periodic timer in milliseconds and counter
		using msg_1 = std::tuple<std::uint32_t, std::size_t>;

		using signatures_t = std::tuple<msg_0, msg_1>;

	public:
		gpio(cyng::async::base_task* bt
			, cyng::logging::log_ptr
			, cyng::filesystem::path path);

		cyng::continuation run();
		void stop(bool shutdown);

		/**
		 * @brief slot [0] - static switch ON/OFF
		 *
		 */
		cyng::continuation process(bool);

		/**
		 * @brief slot [1] - periodic timer in milliseconds and counter
		 *
		 * remove line
		 */
		cyng::continuation process(std::uint32_t, std::size_t);

	private:
		bool control(bool);

	private:
		cyng::async::base_task& base_;

		/**
		 * global logger
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * access path.
		 * example: /sys/class/gpio/gpio50
		 */
		cyng::filesystem::path	path_;

		std::size_t counter_;
		std::chrono::milliseconds ms_;
	};

}

#endif