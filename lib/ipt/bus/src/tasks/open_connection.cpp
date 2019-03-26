/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "open_connection.h"
#include <smf/ipt/response.hpp>
#include <cyng/vm/generator.h>
#include <cyng/intrinsics/op.h>

namespace node
{
	open_connection::open_connection(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, std::string const& number
		, std::chrono::seconds timeout
		, std::size_t retries
		, ipt::bus_interface& bus)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, number_(number)
		, timeout_(timeout)
		, retries_(retries)
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

	cyng::continuation open_connection::run()
	{	
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< retries_
			<< " more trie(s)");

		if (retries_-- > 0) {

			//
			//	* forward connection open request to device
			//	* store sequence - task relation
			//	* start timer to check connection setup
			//
			vm_.async_run({ cyng::generate_invoke("req.open.connection", number_)
				, cyng::generate_invoke("bus.store.relation", cyng::invoke("ipt.seq.push"), base_.get_id())
				, cyng::generate_invoke("stream.flush")
				, cyng::generate_invoke("log.msg.info", "send req.open.connection", cyng::invoke("ipt.seq.push"), number_) });

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
			<< number_);

		// 
		//	push event to client
		//
		//	* [u8] seq
		//	* [bool] success flag
		//	
		BOOST_ASSERT(seq_ != 0u);
		auto buffer = bus_.on_res_open_connection(seq_, false);

		//
		//	remove from task db
		//
		vm_.async_run(cyng::generate_invoke("bus.remove.relation", base_.get_id()));

		return cyng::continuation::TASK_STOP;
	}

	void open_connection::stop(bool shutdown)
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
	cyng::continuation open_connection::process(ipt::sequence_type seq, bool success)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> bus "
			<< vm_.tag()
			<< " received response *"
			<< +seq);

		// 
		//	push event to client
		//
		//	* [u8] seq
		//	* [bool] success flag
		//	
		BOOST_ASSERT(seq == seq_);
		auto buffer = bus_.on_res_open_connection(seq, success);
		if (success && !buffer.empty()) {
			vm_.async_run(cyng::generate_invoke("ipt.transfer.data", buffer));
			vm_.async_run(cyng::generate_invoke("stream.flush"));
		}
		return cyng::continuation::TASK_STOP;
	}

	//	slot 1
	cyng::continuation open_connection::process(ipt::sequence_type seq)
	{
		BOOST_ASSERT(seq_ == 0u);
		BOOST_ASSERT(seq != 0u);
		seq_ = seq;
		return cyng::continuation::TASK_CONTINUE;
	}

}
