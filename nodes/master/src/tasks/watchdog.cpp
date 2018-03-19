/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "watchdog.h"
#include <smf/cluster/generator.h>
#include "../session.h"
#include <cyng/object_cast.hpp>

namespace node
{
	watchdog::watchdog(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::store::db&
		, cyng::object obj
		, std::chrono::seconds monitor)
	: base_(*btp)
		, logger_(logger)
		, session_obj_(obj)
		, monitor_(monitor)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< cyng::object_cast<session>(obj)->vm_.tag()
			<< " is running");

		
	}

	void watchdog::run()
	{	
		//
		//	send watchdogs
		//
		send_watchdogs();

		//
		//	start monitor
		//
		base_.suspend(monitor_);
	}

	void watchdog::stop()
	{

		//
		//	terminate task
		//
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");

	}

	//	slot 0
	cyng::continuation watchdog::process()
	{
		return cyng::continuation::TASK_STOP;
	}

	void watchdog::send_watchdogs()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< cyng::object_cast<session>(session_obj_)->vm_.tag()
			<< " send watchdog");


		cyng::object_cast<session>(session_obj_)->vm_.async_run(bus_req_watchdog());

	}

}
