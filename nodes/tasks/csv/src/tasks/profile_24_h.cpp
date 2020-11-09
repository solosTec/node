/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <tasks/profile_24_h.h>
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

	void profile_24_h::stop(bool shutdown)
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
		std::tm now = cyng::chrono::make_utc_tm(std::chrono::system_clock::now());

		return cyng::chrono::init_tp(cyng::chrono::year(now)
			, cyng::chrono::month(now)
			, cyng::chrono::day(now)
			, 0	//	hour
			, 0	//	minute
			, 0.0) //	this day
			+ std::chrono::hours(24)	//	next day
			;
	}

	void profile_24_h::generate_last_period()
	{
		std::tm const trigger_tp = cyng::chrono::make_utc_tm(next_trigger_tp_);
		if (trigger_tp.tm_mday < 4) {

			//
			//	generate a file for the last month during the first 3 days of the new month
			//

			//
            //	calculate last day of previous month:
            //  * tmp is 3. oct 7:00
            //  * tp is (3. oct 7:00) - (3*24h) => (30. sep 7:00)
			//
			auto const end = next_trigger_tp_ - std::chrono::hours(trigger_tp.tm_mday * 24);
			auto const d = cyng::chrono::days_of_month(end);
			auto const start = end - d;

			//std::cerr << std::endl << "start: " << cyng::to_str(start) << std::endl;

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> generate report for last month ["
				<< cyng::chrono::month(trigger_tp)
                << "] over the last "
                << d.count()
				<< " days" );

			std::tm tmp = cyng::chrono::make_utc_tm(end);

			base_.mux_.post(tsk_db_, 2
				, cyng::tuple_factory(static_cast<std::int32_t>(cyng::chrono::year(tmp))
					, static_cast<std::int32_t>(cyng::chrono::month(tmp))
					, start
					, end + offset_ + std::chrono::hours(24)));
		}

        //
        //	generate a file for the running month
        //

        //
        //	get current day
        //
        auto const end = std::chrono::system_clock::now();
		auto const start = end - std::chrono::hours(trigger_tp.tm_mday * 24);

        CYNG_LOG_INFO(logger_, "task #"
            << base_.get_id()
            << " <"
            << base_.get_class_name()
            << "> generate report for this month ["
            << cyng::chrono::month(trigger_tp)
            << "] over the last "
            << trigger_tp.tm_mday
            << " days" );

        base_.mux_.post(tsk_db_, 2, cyng::tuple_factory(static_cast<std::int32_t>(cyng::chrono::year(trigger_tp))
			, static_cast<std::int32_t>(cyng::chrono::month(trigger_tp))
			, start
			, end));

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

		//
		//	determine year and month to report
		//
		std::tm tmp = cyng::chrono::make_utc_tm(next_trigger_tp_);

		//
		//	calculate time frame
		//
		auto const now = std::chrono::system_clock::now();
		auto const start = now - d;

		base_.mux_.post(tsk_db_, 2, cyng::tuple_factory(static_cast<std::int32_t>(cyng::chrono::year(tmp))
			, static_cast<std::int32_t>(cyng::chrono::month(tmp))
			, start
			, now));

	}

}
