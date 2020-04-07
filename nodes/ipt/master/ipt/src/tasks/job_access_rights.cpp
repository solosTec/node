/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "job_access_rights.h"
#include <cyng/io/io_chrono.hpp>

namespace node
{
	job_access_rights::job_access_rights(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger)
	: base_(*btp)
		, logger_(logger)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> ");
	}

	cyng::continuation job_access_rights::run()
	{	
		//	nothing todo yet
		return cyng::continuation::TASK_STOP;
	}

	void job_access_rights::stop(bool shutdown)
	{
		//
		//	Be careful when using resources from the session like vm_, 
		//	they may be already be invalid.
		//

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");
	}

	//	slot 0 - activity
	cyng::continuation job_access_rights::process()
	{
		return cyng::continuation::TASK_CONTINUE;
	}


}
