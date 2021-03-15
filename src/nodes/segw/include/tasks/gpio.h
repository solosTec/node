/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_GPIO_H
#define SMF_SEGW_TASK_GPIO_H

#include <cyng/task/task_fwd.h>
#include <cyng/log/logger.h>

#include <filesystem>

namespace smf
{
	/**
	 * control GPIO LED
	 */
	class gpio
	{
		template <typename T >
		friend class cyng::task;

		using signatures_t = std::tuple<
			std::function<void(cyng::eod)>,
			std::function<bool(bool)>,	//	turn
			std::function<void(std::chrono::milliseconds, std::size_t)>	//	flashing
		>;

		enum class state {
			OFF,
			FLASHING,
			BLINKING,
		} state_;

	public:
		gpio(std::weak_ptr<cyng::channel>
			, cyng::logger
			, std::filesystem::path);



	private:
		void stop(cyng::eod);

		/**
		 * @brief slot [0] - static turn ON/OFF
		 *
		 */
		bool turn(bool);

		/**
		 * @brief slot [1] - periodic timer in milliseconds and counter
		 *
		 */
		void flashing(std::chrono::milliseconds, std::size_t);

		/**
		 * @brief slot [2] - permanent blinking
		 *
		 */
		void blinking(std::chrono::milliseconds);

	private:
		signatures_t sigs_;
		std::weak_ptr<cyng::channel> channel_;

		/**
		 * global logger
		 */
		cyng::logger logger_;

		/**
		 * access path.
		 * example: /sys/class/gpio/gpio50
		 */
		std::filesystem::path	const path_;

	};

	/**
	 * Set GPIO to the specified state
	 */
	bool switch_gpio(std::filesystem::path, bool state);

	/**
	 * @return true if GPIO is on
	 */
	bool is_gpio_on(std::filesystem::path);

	/**
	 * flip status of specified GPIO path
	 */
	void flip_gpio(std::filesystem::path);
}

#endif