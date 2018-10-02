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
		, std::size_t retries)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, number_(number)
		, timeout_(timeout)
		, retries_(retries)
		//, is_waiting_(false)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
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
			<< " are left over");

		if (retries_-- > 0) {

			//
			//	update task state
			//
			//is_waiting_ = true;

			//
			//	* forward connection open request to device
			//	* store sequence - task relation
			//	* start timer to check connection setup
			//
			vm_	.async_run(cyng::generate_invoke("req.open.connection", number_))
				.async_run(cyng::generate_invoke("bus.store.relation", cyng::invoke("ipt.seq.push"), base_.get_id()))
				.async_run(cyng::generate_invoke("stream.flush"))
				.async_run(cyng::generate_invoke("log.msg.info", "send req.open.connection", cyng::invoke("ipt.seq.push"), number_))
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

		vm_.async_run(cyng::generate_invoke("ipt.res.open.connection"
#if defined(CYNG_LEGACY_MODE_ON)
            , cyng::IDENT
#else
            , cyng::code::IDENT
#endif
			, ipt::sequence_type(0)
			, ipt::response_type(ipt::tp_res_open_connection_policy::UNREACHABLE)));

		return cyng::continuation::TASK_STOP;
	}

	void open_connection::stop()
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
	cyng::continuation open_connection::process(bool success)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> bus "
			<< vm_.tag()
			<< " received response");

        boost::ignore_unused(success);
		return cyng::continuation::TASK_STOP;
	}

}

//#include <cyng/async/task/task.hpp>

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
