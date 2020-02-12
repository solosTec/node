/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "open_channel.h"
#include <smf/ipt/response.hpp>
#include <cyng/vm/generator.h>
#include <cyng/intrinsics/op.h>

namespace node
{
	open_channel::open_channel(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, std::string const& target
		, std::string const& account
		, std::string const& msisdn
		, std::string const& version
		, std::string const& device
		, std::uint16_t timeout
		, ipt::bus_interface& bus
		, std::size_t tsk)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, target_(target)
		, account_(account)
		, msisdn_(msisdn)
		, version_(version)
		, device_(device)
		, timeout_(timeout)
		, bus_(bus)
		, task_(tsk)
		, seq_(0)
		, waiting_(true)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< ':'
			<< vm_.tag()
			<< " <"
			<< base_.get_class_name()
			<< "> is waiting for "
			<< timeout_
			<< " seconds");

	}

	cyng::continuation open_channel::run()
	{	
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< target_);


		if (waiting_) {

			waiting_ = false;

			//
			//	* forward channel open request to device
			//	* store sequence - task relation
			//	* start timer to check connection setup
			//
			vm_.async_run({ cyng::generate_invoke("req.open.push.channel", target_, account_, msisdn_, version_, device_, timeout_)
				, cyng::generate_invoke("bus.store.relation", cyng::invoke("ipt.seq.push"), base_.get_id())
				, cyng::generate_invoke("stream.flush")
				, cyng::generate_invoke("log.msg.info", "send req.open.push.channel(seq: ", cyng::invoke("ipt.seq.push"), ", target: ", target_, ")") });

			//
			//	start monitor
			//
			base_.suspend(std::chrono::seconds(timeout_));

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
			<< target_);

		// 
		//	push event to client
		//
		//	* [u8] seq
		//	* [bool] success flag
		//	
		BOOST_ASSERT(seq_ != 0u);
		if (task_ != cyng::async::NO_TASK) {

			//
			//	forward event
			//
			base_.mux_.post(task_, 0u, cyng::tuple_factory(false, 0, 0, 0, 0));
		}
		bus_.on_res_open_push_channel(seq_, false, 0, 0, 0, 0);

		return cyng::continuation::TASK_STOP;
	}

	void open_channel::stop(bool shutdown)
	{
		//
		//	remove from task db
		//
		vm_.async_run(cyng::generate_invoke("bus.remove.relation", base_.get_id()));

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
	cyng::continuation open_channel::process(bool success
		, std::uint32_t channel
		, std::uint32_t source
		, std::uint16_t status
		, std::uint32_t count)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> bus "
			<< vm_.tag()
			<< " received response "
			<< success);

		// 
		//	push event to client
		//
		//	* [u8] seq
		//	* [bool] success flag
		//	
		if (task_ != cyng::async::NO_TASK)	{

			//
			//	forward event
			//
			base_.mux_.post(task_, 0u, cyng::tuple_factory(success, channel, source, status, count));
		}
		bus_.on_res_open_push_channel(seq_, success, channel, source, status, count);
		return cyng::continuation::TASK_STOP;
	}

	//	slot 1
	cyng::continuation open_channel::process(ipt::sequence_type seq)
	{
		BOOST_ASSERT(seq_ == 0u);
		BOOST_ASSERT(seq != 0u);
		seq_ = seq;
		return cyng::continuation::TASK_CONTINUE;
	}

}
