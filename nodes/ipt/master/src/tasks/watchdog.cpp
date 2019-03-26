/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "watchdog.h"
#include <cyng/io/io_chrono.hpp>
#include <cyng/vm/generator.h>

namespace node
{
	watchdog::watchdog(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, std::uint16_t watchdog
		, std::string account)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, watchdog_(watchdog * 60u + 4)	//!< watchdog in minutes + 4 seconds delay 
		, last_activity_(std::chrono::system_clock::now())
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< account
			<< " watchdog "
			<< cyng::to_str(watchdog_));
	}

	cyng::continuation watchdog::run()
	{	
		const auto delta = std::chrono::system_clock::now() - last_activity_;
		if (delta > watchdog_) {

			//
			//	watchdog timeout - stop session
			//
			CYNG_LOG_ERROR(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> watchdog timeout "
				<< cyng::to_str(delta));

			//
			//	Safe way to intentionally close this session.
			//	
			//	* set session in shutdown state (pending_ = true)
			//	* close socket
			//	* update cluster master state and
			//	* remove session from IP-T masters client_map
			//
			vm_.async_run({ cyng::generate_invoke("session.state.pending")
				, cyng::generate_invoke("ip.tcp.socket.shutdown")
				, cyng::generate_invoke("ip.tcp.socket.close") });

			return cyng::continuation::TASK_STOP;

		}
		else {

			//
			//	re/start monitor
			//	To subtract the delta from the watchdog prevents a time delay
			//	in detecting inresponsive devices
			//
			base_.suspend(watchdog_ - delta);
		}
		return cyng::continuation::TASK_CONTINUE;
	}

	void watchdog::stop(bool shutdown)
	{
		//
		//	Be carefull when using resources from the session like vm_, 
		//	they may already be invalid.
		//

		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> stopped");
	}

	//	slot 0 - activity
	cyng::continuation watchdog::process()
	{
		last_activity_ = std::chrono::system_clock::now();
		return cyng::continuation::TASK_CONTINUE;
	}


}
