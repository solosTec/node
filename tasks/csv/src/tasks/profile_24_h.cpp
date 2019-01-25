/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "profile_24_h.h"
#include <cyng/chrono.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/async/task/base_task.h>
#include <cyng/factory/set_factory.h>
#include <cyng/dom/algorithm.h>

namespace node
{
	profile_24_h::profile_24_h(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::size_t tsk_db
		, std::chrono::minutes offset
		, std::chrono::minutes frame
		, std::string format)
	: base_(*btp)
		, logger_(logger)
		, tsk_db_(tsk_db)
		, offset_(offset)
		, frame_(frame)
		, format_(format)
		, state_(TASK_STATE_INITIAL_)
		//
		//	calculate trigger time
		//
		, next_trigger_tp_(calculate_trigger_tp())
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> next trigger time: "
			<< cyng::to_str(next_trigger_tp_+ offset_));
	}

	cyng::continuation profile_24_h::run()
	{	
		switch (state_) {
		case TASK_STATE_INITIAL_:

			//
			//	trigger generation of last time period
			//
			generate_last_period();


			//
			//	update task state
			//
			state_ = TASK_STATE_RUNNING_;
			break;
		default:
			//
			//	trigger generation of current period
			//
			generate_current_period();
			next_trigger_tp_ += std::chrono::hours(24);
			break;
		}

		//
		//	(re)start timer
		//
		base_.suspend_until(next_trigger_tp_ + offset_);

		return cyng::continuation::TASK_CONTINUE;
	}

	void profile_24_h::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");
	}

	//	slot 0
	cyng::continuation profile_24_h::process()
	{	//	unused
		return cyng::continuation::TASK_CONTINUE;
	}

	std::chrono::system_clock::time_point profile_24_h::calculate_trigger_tp()
	{
		std::tm tmp = cyng::chrono::make_utc_tm(std::chrono::system_clock::now());

		return cyng::chrono::init_tp(cyng::chrono::year(tmp)
			, cyng::chrono::month(tmp)
			, cyng::chrono::day(tmp)
			, 0	//	hour
			, 0	//	minute
			, 0.0) //	this day
			+ std::chrono::hours(24)	//	next day
			;
	}

	void profile_24_h::generate_last_period()
	{
		std::tm tmp = cyng::chrono::make_utc_tm(next_trigger_tp_);
		if (tmp.tm_mday < 4) {

			//
			//	generate a file for the last month during the first 3 days of the new month
			//

			//
            //	calculate last day of previous month:
            //  * tmp is 3. oct 7:00
            //  * tp is (3. oct 7:00) - (3*24h) => (30. sep 7:00)
			//
			auto const tp = next_trigger_tp_ - std::chrono::hours(tmp.tm_mday * 24);
			auto const d = cyng::chrono::days_of_month(tp) + std::chrono::hours(24);

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> generate report for last month ["
				<< cyng::chrono::month(tmp)
                << "] over the last "
                << d.count()
				<< " days" );

            base_.mux_.post(tsk_db_, 2, cyng::tuple_factory(tp, static_cast<std::int32_t>(d.count())));

		}
        else {

            //
            //	generate a file for the running month
            //

            //
            //	get current day
            //
            auto tp = std::chrono::system_clock::now();

            CYNG_LOG_INFO(logger_, "task #"
                << base_.get_id()
                << " <"
                << base_.get_class_name()
                << "> generate report for this month ["
                << cyng::chrono::month(tmp)
                << "] over the last "
                << tmp.tm_mday
                << " days" );

            base_.mux_.post(tsk_db_, 2, cyng::tuple_factory(tp, static_cast<std::int32_t>(tmp.tm_mday)));

        }
	}

	void profile_24_h::generate_current_period()
	{
		auto d = cyng::chrono::days_of_month(next_trigger_tp_);

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> generate report for this month with "
			<< d.count()
			<< " days");

		base_.mux_.post(tsk_db_, 2, cyng::tuple_factory(next_trigger_tp_, d.count()));

	}

}
