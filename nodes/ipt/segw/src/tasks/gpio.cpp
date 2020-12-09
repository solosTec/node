﻿/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include <tasks/gpio.h>

#include <fstream>
#include <iostream>

namespace node
{
	gpio::gpio(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::filesystem::path p)
	: base_(*btp) 
		, logger_(logger)
		, path_(p)
		, counter_(5)
		, ms_(10)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< path_.generic_string());

		//
		//	turn LED off
		//
		control(false);
	}

	cyng::continuation gpio::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> counter: "
			<< counter_);
#endif

		if (counter_ != 0) {
			--counter_;
			base_.suspend(ms_);

            CYNG_LOG_DEBUG(logger_, "task #"
                << base_.get_id()
                << " <"
                << base_.get_class_name()
                << "> counter: "
                << counter_);
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
		auto const p = (path_ / "value").generic_string();
        
		CYNG_LOG_DEBUG(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
            << p
            << (on ? " on" : " off"));
        

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

