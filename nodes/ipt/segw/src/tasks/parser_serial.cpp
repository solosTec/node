/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "parser_serial.h"
#include "../cache.h"

#include <iostream>

namespace node
{
	parser_serial::parser_serial(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, cache& cfg)
	: base_(*btp) 
		, logger_(logger)
		, cache_(cfg)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
	}

	cyng::continuation parser_serial::run()
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

		return cyng::continuation::TASK_CONTINUE;
	}

	void parser_serial::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation parser_serial::process()
	{

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation parser_serial::process(bool b)
	{
		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}


}

