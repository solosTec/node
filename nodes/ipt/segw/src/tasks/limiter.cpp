/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "limiter.h"
#include "../bridge.h"
#include <smf/sml/event.h>
#include <smf/sml/obis_db.h>

#include <cyng/io/io_chrono.hpp>

namespace node
{
	limiter::limiter(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, bridge& com
		, std::chrono::hours interval_time)
	: base_(*btp) 
		, logger_(logger)
		, bridge_(com)
		, interval_time_(interval_time)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> with "
			<< cyng::to_str(interval_time_)
			<< " interval");
	}

	cyng::continuation limiter::run()
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

		//
		//	get maximum size of all data collectors and shrink tables
		//	if maximum size was exceeded.
		//
		bridge_.shrink();

		return cyng::continuation::TASK_CONTINUE;
	}

	void limiter::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation limiter::process()
	{

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

}

