/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "storage_xml.h"
#include <cyng/async/task/task_builder.hpp>
#include <cyng/dom/reader.h>
#include <cyng/io/serializer.h>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	storage_xml::storage_xml(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::store::db& cache
		, cyng::param_map_t)
	: base_(*btp)
		, logger_(logger)
		, cache_(cache)
	{
		CYNG_LOG_INFO(logger_, "task #"
		<< base_.get_id()
		<< " <"
		<< base_.get_class_name()
		<< "> is running");
	}

	cyng::continuation storage_xml::run()
	{	
		CYNG_LOG_INFO(logger_, "storage_xml is running");

		return cyng::continuation::TASK_CONTINUE;
	}

	void storage_xml::stop()
	{
		CYNG_LOG_INFO(logger_, "storage_xml is stopped");
	}

	//	slot 0
	cyng::continuation storage_xml::process(int i, std::string name)
	{
		//std::cout << "simple::slot-0($" << base_->get_id() << ", " << i << ", " << name << ")" << std::endl;
		return cyng::continuation::TASK_CONTINUE;
	}


}
