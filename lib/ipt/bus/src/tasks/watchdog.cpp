/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "watchdog.h"
#include <smf/ipt/response.hpp>
#include <smf/ipt/bus.h>
#include <cyng/vm/generator.h>
#include <cyng/intrinsics/op.h>

namespace node
{
	watchdog::watchdog(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, std::uint16_t watchdog)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, watchdog_(watchdog)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< ':'
			<< vm_.tag()
			<< " <"
			<< base_.get_class_name()
			<< "> with "
			<< watchdog_
			<< " minutes");
	}

	cyng::continuation watchdog::run()
	{	

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> register target "
			<< name_);

		//
		//	re/start monitor
		//
		base_.suspend(std::chrono::minutes(watchdog_));

		//
		//	send watchdog
		//
		vm_.async_run({ cyng::generate_invoke("res.watchdog", static_cast<std::uint8_t>(0)), cyng::generate_invoke("stream.flush") });
		return cyng::continuation::TASK_CONTINUE;
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
}
