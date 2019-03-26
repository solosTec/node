/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "close_connection.h"
#include <smf/ipt/response.hpp>
#include <cyng/vm/generator.h>

namespace node
{
	close_connection::close_connection(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, std::chrono::seconds timeout
		, ipt::bus_interface& bus)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
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

	cyng::continuation close_connection::run()
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
				<< "> sends close connection request");

			//
			//	* forward connection open request to device
			//	* store sequence - task relation
			//	* start timer to check connection setup
			//
			vm_.async_run({ cyng::generate_invoke("req.close.connection")
				, cyng::generate_invoke("bus.store.relation", cyng::invoke("ipt.seq.push"), base_.get_id())
				, cyng::generate_invoke("stream.flush")
				, cyng::generate_invoke("log.msg.info", "send req.close.connection", cyng::invoke("ipt.seq.push")) });

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

		// 
		//	push event to client
		//
		//	* [u8] seq
		//	* [bool] success flag
		//	
		BOOST_ASSERT(seq_ != 0u);
		bus_.on_res_close_connection(seq_);

		//
		//	remove from task db
		//
		vm_.async_run(cyng::generate_invoke("bus.remove.relation", base_.get_id()));

		return cyng::continuation::TASK_STOP;
	}

	void close_connection::stop(bool shutdown)
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
	cyng::continuation close_connection::process(ipt::sequence_type seq, bool success)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> bus "
			<< vm_.tag()
			<< " received response");

		//
		//	call client event
		//
		BOOST_ASSERT(seq == seq_);
		bus_.on_res_close_connection(seq);

		return cyng::continuation::TASK_STOP;
	}

	//	slot 1
	cyng::continuation close_connection::process(ipt::sequence_type seq)
	{
		BOOST_ASSERT(seq_ == 0u);
		BOOST_ASSERT(seq != 0u);
		seq_ = seq;
		return cyng::continuation::TASK_CONTINUE;
	}

}

