/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "storage_json.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	storage_json::storage_json(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::store::db& cache
		, cyng::param_map_t)
	: base_(*btp)
	, logger_(logger)
	, cache_(cache)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
	}

	cyng::continuation storage_json::run()
	{	
		CYNG_LOG_INFO(logger_, "storage_json is running");
		return cyng::continuation::TASK_CONTINUE;
	}

	void storage_json::stop()
	{
		CYNG_LOG_INFO(logger_, "storage_json is stopped");
	}

	//	slot 0
	cyng::continuation storage_json::process(int i, std::string name)
	{
		//std::cout << "simple::slot-0($" << base_->get_id() << ", " << i << ", " << name << ")" << std::endl;
		return cyng::continuation::TASK_CONTINUE;
	}


}
