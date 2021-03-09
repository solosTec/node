/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/gpio.h>
#include <cyng/log/record.h>

#include <fstream>
#include <iostream>

namespace smf
{
	gpio::gpio(std::weak_ptr<cyng::channel> wp
		, cyng::logger logger
		, std::filesystem::path p)
		: sigs_{
			std::bind(&gpio::stop, this, std::placeholders::_1),
			std::bind(&gpio::turn, this, std::placeholders::_1),
			std::bind(&gpio::flashing, this, std::placeholders::_1, std::placeholders::_2)
	}	, channel_(wp)
		, logger_(logger)
		, path_(p)
		, counter_(5)
		, ms_(10)
	{
		CYNG_LOG_INFO(logger_, "GPIO [" << path_ << "] ready");

		//
		//	turn LED off
		//
		turn(false);
	}

	void gpio::stop(cyng::eod) {
		CYNG_LOG_INFO(logger_, "GPIO [" << path_ << "] stopped");
		boost::system::error_code ec;
	}

//	cyng::continuation gpio::run()
//	{
//#ifdef _DEBUG
//		CYNG_LOG_TRACE(logger_, "task #"
//			<< base_.get_id()
//			<< " <"
//			<< base_.get_class_name()
//			<< "> counter: "
//			<< counter_);
//#endif
//
//		if (counter_ != 0) {
//			--counter_;
//			base_.suspend(ms_);
//
//            CYNG_LOG_DEBUG(logger_, "task #"
//                << base_.get_id()
//                << " <"
//                << base_.get_class_name()
//                << "> counter: "
//                << counter_);
//			//
//			//	last value of counter_ is 0 which turns the LED off
//			//
//			control((counter_ % 2) != 0);
//		}

	//	return cyng::continuation::TASK_CONTINUE;
	//}


	/**
	 * @brief slot [1] - periodic timer in milliseconds and counter
	 *
	 */
	void gpio::flashing(std::uint32_t ms, std::size_t counter)
	{
		counter_ = counter;
		ms_ = std::chrono::milliseconds(ms);

		//base_.suspend(ms_);
		turn((counter_ % 2) == 0);

	}

	bool gpio::turn(bool on)
	{
		//
		//	build complete path
		//
		auto const p = (path_ / "value").generic_string();
        
		CYNG_LOG_DEBUG(logger_, "GPIO [" << path_ << "] ready" << (on ? " on" : " off"));
        
		if (switch_gpio(p, on))	return true;

		CYNG_LOG_WARNING(logger_, "cannot open GPIO [" << p << "]");
		return false;
	}

	bool switch_gpio(std::filesystem::path p, bool on) {
		std::fstream ifs(p, std::ios_base::in | std::ios_base::out);
		if (ifs.is_open()) {
			ifs << (on ? 1 : 0);
			return true;
		}
		return false;

	}
}

