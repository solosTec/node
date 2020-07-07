/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "clock.h"
#include "../cache.h"

#include <cyng/parser/chrono_parser.h>
#if BOOST_OS_LINUX
#include <cyng/sys/rtc.h>
#endif

#include <cyng/io/io_chrono.hpp>

namespace node
{
	clock::clock(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cache& cfg
		, std::chrono::minutes interval_time)
		: base_(*btp)
		, logger_(logger)
		, cache_(cfg)
		, interval_time_(interval_time)
		, initialized_(false)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> with "
			<< cyng::to_str(interval_time_)
			<< " interval");
	}

	cyng::continuation clock::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
#endif
		//
		//	start/restart timer
		//
		base_.suspend(interval_time_);

		if (initialized_) {
			//
			//	update "time-offset"
			//
			cache_.set_cfg("time-offset", std::chrono::system_clock::now());
		}
		else {

			//
			//	init cache and database values
			//
			init();

			//
			//	update task state
			//
			initialized_ = true;
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void clock::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation clock::process()
	{
		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	void clock::init()
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> initialize time offset");

		cache_.write_table("_Cfg", [&](cyng::store::table* tbl) {

			//
			//	check time-offset
			//
			auto const tof_key = cyng::table::key_generator("time-offset");
			if (!tbl->exist(tof_key)) {

				//
				//	if this entry doesn't exist, we have to create one.
				//

				auto const r = cyng::parse_rfc3339_timestamp(NODE_BUILD_DATE);

				CYNG_LOG_WARNING(logger_, "merge time-offset "
					<< cyng::to_str(r.second ? r.first : std::chrono::system_clock::now()));

				tbl->merge(tof_key
					, cyng::table::data_generator(r.second ? r.first : std::chrono::system_clock::now())
					, 1u
					, cache_.get_tag());

			}
		});


		//
		//	check system clock
		//
		auto const tof = cache_.get_cfg("time-offset", std::chrono::system_clock::now());
		auto const now = std::chrono::system_clock::now();
		if (now < tof) {

			//
			//	System clock has a time before the last time tag.
			//	Set a more recent timestamp.
			//
			CYNG_LOG_WARNING(logger_, "RTC is slow/wrong: "
				<< cyng::to_str(now)
				<< " < "
				<< cyng::to_str(tof));

		}
#if defined(NODE_CROSS_COMPILE)
		if (!cyng::sys::write_hw_clock(tof, 0)) {
			CYNG_LOG_ERROR(logger_, "set RTC failed: "
				<< cyng::to_str(tof));

			//
			//  synchronize RTC to system clock
			//  hwclock --hctosys --utc
			//
			system("hwclock --hctosys --utc");
		}
		else {
			CYNG_LOG_INFO(logger_, "set RTC: "
				<< cyng::to_str(tof));
		}
#endif

	}

}

