/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "multiple.h"
#include <cyng/chrono.h>
#include <cyng/io/io_chrono.hpp>
#include <cyng/async/task/base_task.h>
#include <cyng/factory/set_factory.h>
#include <cyng/dom/algorithm.h>

namespace node
{
	multiple::multiple(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, boost::filesystem::path out
		, std::string prefix
		, std::string suffix
		, std::chrono::seconds period)
	: base_(*btp)
		, logger_(logger)
		, out_(out)
		, period_(period)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
	}

	cyng::continuation multiple::run()
	{	
		//
		//	(re)start timer
		//
		base_.suspend(period_);
		return cyng::continuation::TASK_CONTINUE;
	}

	void multiple::stop()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");
	}

	cyng::continuation multiple::process(std::string table
		, std::size_t idx
		, std::chrono::system_clock::time_point	tp
		, boost::uuids::uuid tag
		, std::string name
		, std::string evt
		, std::string descr)
	{
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> idx: "
			<< idx);

		return cyng::continuation::TASK_CONTINUE;
	}


}
