/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "gpio.h"
#include <fstream>
#include <iostream>

namespace node
{
	gpio::gpio(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::filesystem::path path)
	: base_(*btp) 
		, logger_(logger)
		, path_(path)
		, counter_(5)
		, ms_(10)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

		//
		//	turn LED off
		//
		control(false);
	}

	cyng::continuation gpio::run()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> counter: "
			<< counter_);

		if (counter_ != 0) {
			--counter_;
			base_.suspend(ms_);

			//
			//	last value of counter_ is 0 which turns the LED off
			//
			control((counter_ % 2) != 0);
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void gpio::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
            << path_
            << " is stopped");
	}

	cyng::continuation gpio::process(bool on)
	{
		CYNG_LOG_DEBUG(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
            << path_
            << (on ? " on" : " oj   off"));

        control(on);

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	/**
	 * @brief slot [1] - periodic timer in milliseconds and counter
	 *
	 * remove line
	 */
	cyng::continuation gpio::process(std::uint32_t ms, std::size_t counter)
	{
		counter_ = counter;
		ms_ = std::chrono::milliseconds(ms);

		base_.suspend(ms_);
		control((counter_ % 2) == 0);

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	bool gpio::control(bool on)
	{
		//
		//	build complete path
		//
		auto const p = (path_ / "value").string();

		std::fstream ifs(p, std::ios_base::in | std::ios_base::out);
		if (ifs.is_open()) {
			ifs << (on ? 1 : 0);
			return true;
		}

		CYNG_LOG_ERROR(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> cannot open "
			<< p);

		return false;
	}
}

