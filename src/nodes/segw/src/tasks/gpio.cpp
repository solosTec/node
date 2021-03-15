﻿/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/gpio.h>
#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>

#include <fstream>
#include <iostream>

namespace smf
{
	gpio::gpio(std::weak_ptr<cyng::channel> wp
		, cyng::logger logger
		, std::filesystem::path p)
		: state_(state::OFF)
		, sigs_{
			std::bind(&gpio::stop, this, std::placeholders::_1),	//	0
			std::bind(&gpio::turn, this, std::placeholders::_1),	//	1
			std::bind(&gpio::flashing, this, std::placeholders::_1, std::placeholders::_2)	//	2
		}	
		, channel_(wp)
		, logger_(logger)
		, path_(p)
	{
		CYNG_LOG_INFO(logger_, "GPIO [" << path_ << "] ready");

		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("turn", 1);
			sp->set_channel_name("flashing", 2);
		}

		//
		//	turn LED off
		//
		turn(false);
	}

	void gpio::stop(cyng::eod) {
		CYNG_LOG_INFO(logger_, "GPIO [" << path_ << "] stopped");
		boost::system::error_code ec;
	}

	/**
	 * @brief slot [1] - periodic timer in milliseconds and counter
	 *
	 */
	void gpio::flashing(std::chrono::milliseconds ms, std::size_t counter)
	{
		//
		//	complete
		//
		if (counter == 0)	return;

		turn((counter % 2) == 0);

		auto sp = channel_.lock();
		if (sp)	sp->suspend(ms, 2, cyng::make_tuple(ms, counter - 1));

	}

	void gpio::blinking(std::chrono::milliseconds ms) {
		auto sp = channel_.lock();
		flip_gpio(path_);
		if (sp)	sp->suspend(ms, 3, cyng::make_tuple(ms));
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
		std::ofstream ofs(p);
		if (ofs.is_open()) {
			ofs << (on ? 1 : 0);
			return true;
		}
		return false;
	}

	bool is_gpio_on(std::filesystem::path p) {
		std::ifstream ifs(p);
		if (ifs.is_open()) {
			int state = 0;
			ifs >> state;
			return state != 0;
		}
		return false;
	}

	void flip_gpio(std::filesystem::path p) {
		std::fstream fs(p, std::ios_base::in | std::ios_base::out);
		if (fs.is_open()) {
			int state = 0;
			fs >> state;
			fs << ((state == 0) ? 1 : 0);
		}
	}
}

