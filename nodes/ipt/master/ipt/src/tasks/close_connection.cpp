/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "close_connection.h"
#include <smf/cluster/generator.h>
#include <smf/ipt/response.hpp>
#include <cyng/async/task/base_task.h>
#include <cyng/io/serializer.h>
#include <cyng/dom/reader.h>
#include <cyng/vm/generator.h>
#include <boost/uuid/random_generator.hpp>

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
		, start_(std::chrono::system_clock::now())
		, is_waiting_(false)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< vm_.tag()
			<< " is running until "
			<< cyng::to_str(start_ + timeout_));
	}

	cyng::continuation close_connection::run()
	{	
		if (!is_waiting_)
		{
			//
			//	update task state
			//
			is_waiting_ = true;

			//
			//	* forward connection close request to device
			//	* store sequence - task relation
			//	* start timer to check connection setup
			//

			//	prepare IP-T command: close connection request
			vm_.async_run({ cyng::generate_invoke("req.close.connection")

				//	tie IP-T sequence with this task id
				, cyng::generate_invoke("session.store.relation", cyng::invoke("ipt.seq.push"), base_.get_id(), 0u)

				//	send IP-T request
				, cyng::generate_invoke("stream.flush")

				//	logging
				, cyng::generate_invoke("log.msg.info", "client.req.close.connection.forward", cyng::invoke("ipt.seq.push"), timeout_) });

			//
			//	start monitor
			//
			base_.suspend(timeout_);
			return cyng::continuation::TASK_CONTINUE;

		}

		//
		//	timeout
		//
		CYNG_LOG_WARNING(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< vm_.tag()
			<< " timeout after "
			<< cyng::to_str(std::chrono::system_clock::now() - start_));

		if (vm_.is_halted()) {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> stop (session already halted) "
				<< vm_.tag());

		}
		else
		{
			//
			//	Safe way to intentionally close this session.
			//	
			//	* set session in shutdown state
			//	* close socket
			//	* update cluster master state and
			//	* remove session from IP-T masters client_map
			//
			vm_.async_run({ cyng::generate_invoke("session.state.pending")
				, cyng::generate_invoke("ip.tcp.socket.shutdown")
				, cyng::generate_invoke("ip.tcp.socket.close") });
		}


		//
		//	stop this task
		//
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
	cyng::continuation close_connection::process(ipt::response_type res)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> received response ["
			<< ipt::tp_res_close_connection_policy::get_response_name(res)
			<< "]");

		return cyng::continuation::TASK_STOP;
	}
}

