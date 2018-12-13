/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "open_connection.h"
#include <smf/cluster/generator.h>
#include <smf/ipt/response.hpp>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/dom/reader.h>
#include <boost/uuid/nil_generator.hpp>

namespace node
{
	open_connection::open_connection(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, std::string number
		, std::chrono::seconds timeout)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, number_(number)
		, timeout_(timeout)
		, start_(std::chrono::system_clock::now())
		, is_waiting_(false)
	{
		CYNG_LOG_INFO(logger_, "task #"
		<< base_.get_id()
		<< " <"
		<< base_.get_class_name()
		<< "> is running until "
		<< cyng::to_str(start_ + timeout_));

	}

	cyng::continuation open_connection::run()
	{	
		if (!is_waiting_)
		{
			//
			//	update task state
			//
			is_waiting_ = true;

			//
			//	* forward connection open request to device
			//	* store sequence - task relation
			//	* start timer to check connection setup
			//

			vm_.async_run({ cyng::generate_invoke("req.open.connection", number_)
				, cyng::generate_invoke("session.store.relation", cyng::invoke("ipt.seq.push"), base_.get_id(), 0u)
				, cyng::generate_invoke("stream.flush")
				, cyng::generate_invoke("log.msg.info", "client.req.open.connection.forward", cyng::invoke("ipt.seq.push"), number_) });

			//
			//	start monitor
			//
			base_.suspend(timeout_);

			return cyng::continuation::TASK_CONTINUE;
		}

		CYNG_LOG_WARNING(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> timeout after "
			<< cyng::to_str(std::chrono::system_clock::now() - start_));

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
			<< "> is stopped after "
			<< cyng::to_str(std::chrono::system_clock::now() - start_));

	}

	//	slot 0
	cyng::continuation open_connection::process(ipt::response_type res)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> received response ["
			<< ipt::tp_res_open_connection_policy::get_response_name(res)
			<< "]");

		return cyng::continuation::TASK_STOP;
	}
}
