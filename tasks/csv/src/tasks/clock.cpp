/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "clock.h"
#include <cyng/chrono.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/async/task/base_task.h>
#include <cyng/factory/set_factory.h>
#include <cyng/dom/algorithm.h>

namespace node
{
	clock::clock(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::size_t tsk_db
		, cyng::param_map_t cfg_trigger)
	: base_(*btp)
		, logger_(logger)
		, tsk_db_(tsk_db)
		, offset_(cyng::find_value(cfg_trigger, "offset", 7))
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

	cyng::continuation clock::run()
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

	void clock::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");
	}

	//	slot 0
	cyng::continuation clock::process(cyng::version const& v)
	{
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation clock::process()
	{
		return cyng::continuation::TASK_CONTINUE;
	}

	std::chrono::system_clock::time_point clock::calculate_trigger_tp()
	{
		std::tm tmp = cyng::chrono::make_utc_tm(std::chrono::system_clock::now());

		return cyng::chrono::init_tp(cyng::chrono::year(tmp)
			, cyng::chrono::month(tmp)
			, cyng::chrono::day(tmp)
			, 0	//	hour
			, 0	//	minute
			, 0.0) 
			+ std::chrono::hours(24)	//	next day
			;

	}

	void clock::generate_last_period()
	{
		//
		//	start time of previous period
		//

		base_.mux_.post(tsk_db_, 0, cyng::tuple_factory(next_trigger_tp_ - std::chrono::hours(24), next_trigger_tp_, std::chrono::minutes(15)));
		
	}

	void clock::generate_current_period()
	{
		base_.mux_.post(tsk_db_, 0, cyng::tuple_factory(next_trigger_tp_, next_trigger_tp_ + std::chrono::hours(24), std::chrono::minutes(15)));

	}

}
