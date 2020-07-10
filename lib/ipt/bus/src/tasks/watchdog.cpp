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
#include <cyng/io/io_chrono.hpp>

namespace node
{
	watchdog::watchdog(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, std::uint16_t watchdog)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, watchdog_((watchdog * 60) - 4)	//	send 4 seconds earlier
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< ':'
			<< vm_.tag()
			<< " <"
			<< base_.get_class_name()
			<< "> with "
			<< cyng::to_str(watchdog_)
			<< " watchdog");
	}

	cyng::continuation watchdog::run()
	{	

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> send response");

		//
		//	re/start monitor
		//
		base_.suspend(watchdog_);

		//
		//	send watchdog
		//
		vm_.async_run({ cyng::generate_invoke("res.watchdog", static_cast<ipt::sequence_type>(0u)), cyng::generate_invoke("stream.flush") });
		return cyng::continuation::TASK_CONTINUE;
	}

	void watchdog::stop(bool shutdown)
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
