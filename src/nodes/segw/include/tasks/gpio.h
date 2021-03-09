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
			std::function<void(std::uint32_t, std::size_t)>	//	flashing
		>;

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
		void flashing(std::uint32_t, std::size_t);

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

		std::size_t counter_;
		std::chrono::milliseconds ms_;
	};

	bool switch_gpio(std::filesystem::path, bool);
}

#endif