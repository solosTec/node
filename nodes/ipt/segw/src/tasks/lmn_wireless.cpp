/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "lmn_wired.h"
//#include <fstream>
#include <iostream>

namespace node
{
	lmn_wireless::lmn_wireless(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::filesystem::path path)
	: base_(*btp) 
		, logger_(logger)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");

	}

	cyng::continuation lmn_wireless::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
#endif

		return cyng::continuation::TASK_CONTINUE;
	}

	void lmn_wireless::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation lmn_wireless::process(bool on)
	{

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

}

