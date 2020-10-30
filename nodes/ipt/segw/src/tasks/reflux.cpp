/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include <tasks/reflux.h>
#include <cache.h>

#include <cyng/parser/chrono_parser.h>
#if BOOST_OS_LINUX
#include <cyng/sys/rtc.h>
#endif

#include <cyng/io/io_chrono.hpp>

namespace node
{
	reflux::reflux(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger)
	: base_(*btp)
		, logger_(logger)
		, initialized_(false)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">")

	}

	cyng::continuation reflux::run()
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
		base_.suspend(std::chrono::minutes(5));

		if (initialized_) {
			//
			//	update "time-offset"
			//
			//cache_.set_cfg("time-offset", std::chrono::system_clock::now());
		}
		else {

			//
			//	init cache and database values
			//
			init();

			//
			//	update task state
			//
			initialized_ = true;
		}

		return cyng::continuation::TASK_CONTINUE;
	}

	void reflux::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation reflux::process()
	{
		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

	void reflux::init()
	{

	}

}

