/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "obislog.h"
#include <iostream>

namespace node
{
	obislog::obislog(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, std::chrono::minutes cycle_time)
	: base_(*btp) 
		, logger_(logger)
		, cycle_time_(cycle_time)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
	}

	cyng::continuation obislog::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
#endif
		//
		//	ToDo: write the log entry
		//

		//
		//	start/restart timer
		//
		base_.suspend(cycle_time_);

		return cyng::continuation::TASK_CONTINUE;
	}

	void obislog::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation obislog::process()
	{

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

}

