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
		, std::string const& name
		, std::uint16_t packet_size
		, std::uint8_t window_size
		, std::chrono::seconds timeout)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, name_(name)
		, packet_size_(packet_size)
		, window_size_(window_size)
		, timeout_(timeout)
		, is_waiting_(false)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< ':'
			<< vm_.tag()
			<< " <"
			<< base_.get_class_name()
			<< "> is waiting for "
			<< cyng::to_str(timeout_));

	}

	cyng::continuation watchdog::run()
	{	
		if (!is_waiting_) {

			//
			//	update task state
			//
			is_waiting_ = true;

			CYNG_LOG_INFO(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> register target "
				<< name_);

			//
			//	* forward register target request to device
			//	* store sequence - task relation
			//	* start timer to check connection setup
			//
			vm_	.async_run(cyng::generate_invoke("req.register.push.target", name_, packet_size_, window_size_))
				.async_run(cyng::generate_invoke("bus.store.relation", cyng::invoke("ipt.seq.push"), base_.get_id()))
				.async_run(cyng::generate_invoke("stream.flush"))
				.async_run(cyng::generate_invoke("log.msg.info", "send req.register.push.target", cyng::invoke("ipt.seq.push"), name_))
				;

			//
			//	start monitor
			//
			base_.suspend(timeout_);

			//
			//	start waiting
			//
			return cyng::continuation::TASK_CONTINUE;
		}

		CYNG_LOG_WARNING(logger_, "task #"
			<< base_.get_id()
			<< ':'
			<< vm_.tag()
			<< " <"
			<< base_.get_class_name()
			<< "> timeout");

		vm_.async_run(cyng::generate_invoke("register.push.target.timeout"
#if defined(CYNG_LEGACY_MODE_ON)
            , cyng::IDENT
#else
            , cyng::code::IDENT
#endif
			, base_.get_id()));

		return cyng::continuation::TASK_STOP;
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
	cyng::continuation watchdog::process(ipt::sequence_type seq, bool success, std::uint32_t channel, std::size_t tsk)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> bus "
			<< vm_.tag()
			<< " received response => #"
			<< tsk);

		//base_.mux_.post(tsk
		//	, ipt::bus::ipt_events::IPT_EVENT_PUSH_TARGET_REGISTERED
		//	, cyng::tuple_factory(seq, success, channel, name_));

		return cyng::continuation::TASK_STOP;
	}
}
