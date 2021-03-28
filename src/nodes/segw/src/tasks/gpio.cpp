/*
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
			std::bind(&gpio::flashing, this, std::placeholders::_1, std::placeholders::_2),	//	2
			std::bind(&gpio::blinking, this, std::placeholders::_1)	//	3
		}	
		, channel_(wp)
		, logger_(logger)
		, path_(p / "value")
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("turn", 1);
			sp->set_channel_name("flashing", 2);
			sp->set_channel_name("blinking", 3);
			CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
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
		// CYNG_LOG_TRACE(logger_, "GPIO [" << path_ << "] flashing " << counter);

		//
		//	complete
		//
		if (counter == 0)	return;

		turn((counter % 2) == 0);

		auto sp = channel_.lock();
		if (sp)	sp->suspend(ms, 2, cyng::make_tuple(ms, counter - 1));

	}

	void gpio::blinking(std::chrono::milliseconds ms) {

		flip_gpio(path_);
		auto sp = channel_.lock();
		if (sp)	sp->suspend(ms, 3, cyng::make_tuple(ms));
	}

	bool gpio::turn(bool on)
	{ 
		//
		//	stop active timer
		//
		auto sp = channel_.lock();
		if (sp)	sp->cancel_timer();

		// CYNG_LOG_TRACE(logger_, "turn GPIO [" << path_ << "] " << (on ? "on" : "off"));
        
		if (switch_gpio(path_, on))	return true;

		CYNG_LOG_WARNING(logger_, "cannot open GPIO [" << path_ << "]");
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

