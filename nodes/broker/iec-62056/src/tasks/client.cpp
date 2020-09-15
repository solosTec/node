/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "client.h"
//#include <smf/cluster/generator.h>
//#include <cyng/async/task/task_builder.hpp>
//#include <cyng/io/serializer.h>
//#include <cyng/vm/generator.h>
//#include <boost/uuid/random_generator.hpp>

namespace node
{
	client::client(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger)
	: base_(*btp)
		, logger_(logger)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");


	}

	cyng::continuation client::run()
	{	
		return cyng::continuation::TASK_CONTINUE;
	}

	cyng::continuation client::process()
	{
		return cyng::continuation::TASK_CONTINUE;
	}

	void client::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");
	}

}
