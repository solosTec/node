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
		, std::chrono::seconds timeout)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, timeout_(timeout)
		, is_waiting_(false)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
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

			//
			//	* forward connection open request to device
			//	* store sequence - task relation
			//	* start timer to check connection setup
			//
			vm_	.async_run(cyng::generate_invoke("req.close.connection"))
				.async_run(cyng::generate_invoke("bus.store.relation", cyng::invoke("ipt.seq.push"), base_.get_id()))
				.async_run(cyng::generate_invoke("stream.flush"))
				.async_run(cyng::generate_invoke("log.msg.info", "send req.close.connection", cyng::invoke("ipt.seq.push")))
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
			<< " <"
			<< base_.get_class_name()
			<< "> timeout");

		vm_.async_run(cyng::generate_invoke("ipt.res.close.connection"
			, cyng::IDENT
			, ipt::sequence_type(0)
			, ipt::response_type(ipt::tp_res_close_connection_policy::CONNECTION_CLEARING_FAILED)));

		return cyng::continuation::TASK_STOP;
	}

	void close_connection::stop()
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
	cyng::continuation close_connection::process(bool success)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> bus "
			<< vm_.tag()
			<< " received response");

		return cyng::continuation::TASK_STOP;
	}

}

#include <cyng/async/task/task.hpp>

//namespace cyng {
//	namespace async {
//
//		//
//		//	initialize static slot names
//		//
//		template <>
//		std::map<std::string, std::size_t> cyng::async::task<node::open_connection>::slot_names_({ { "shutdown", 1 } });
//
//	}
//}
