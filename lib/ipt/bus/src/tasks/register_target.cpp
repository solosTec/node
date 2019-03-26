/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "register_target.h"
#include <smf/ipt/response.hpp>
#include <smf/ipt/bus.h>
#include <cyng/vm/generator.h>
#include <cyng/intrinsics/op.h>

namespace node
{
	register_target::register_target(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, std::string const& name
		, std::uint16_t packet_size
		, std::uint8_t window_size
		, std::chrono::seconds timeout
		, ipt::bus_interface& bus)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, name_(name)
		, packet_size_(packet_size)
		, window_size_(window_size)
		, timeout_(timeout)
		, is_waiting_(false)
		, bus_(bus)
		, seq_(0)
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

	cyng::continuation register_target::run()
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
			vm_.async_run({ cyng::generate_invoke("req.register.push.target", name_, packet_size_, window_size_)
				, cyng::generate_invoke("bus.store.relation", cyng::invoke("ipt.seq.push"), base_.get_id())
				, cyng::generate_invoke("stream.flush")
				, cyng::generate_invoke("log.msg.info", "send req.register.push.target", cyng::invoke("ipt.seq.push"), name_) });

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
			<< "> timeout - "
			<< name_);

		//
		//	push event to client
		//
		BOOST_ASSERT(seq_ != 0u);
		bus_.on_res_register_target(seq_
			, false
			, 0
			, name_);

		//
		//	remove from task db
		//
		vm_.async_run(cyng::generate_invoke("bus.remove.relation", base_.get_id()));

		return cyng::continuation::TASK_STOP;
	}

	void register_target::stop(bool shutdown)
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
	cyng::continuation register_target::process(ipt::sequence_type seq, bool success, std::uint32_t channel)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> bus "
			<< vm_.tag()
			<< " received response: "
			<< name_
			<< ':'
			<< channel);

		//
		//	push event to client
		//
		BOOST_ASSERT(seq == seq_);
		bus_.on_res_register_target(seq
			, success
			, channel
			, name_);

		return cyng::continuation::TASK_STOP;
	}

	//	slot 1
	cyng::continuation register_target::process(ipt::sequence_type seq)
	{
		BOOST_ASSERT(seq_ == 0u);
		BOOST_ASSERT(seq != 0u);
		seq_ = seq;
		return cyng::continuation::TASK_CONTINUE;
	}

}
