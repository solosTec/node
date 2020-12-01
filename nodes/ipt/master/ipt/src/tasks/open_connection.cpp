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
		, is_initialized_(false)
	{
		CYNG_LOG_INFO(logger_, "task #"
		<< base_.get_id()
		<< " <"
		<< base_.get_class_name()
		<< "> "
		<< vm_.tag()
		<< " is running for "
		<< cyng::to_str(timeout_)
		<< " until "
		<< cyng::to_str(start_ + timeout_));

	}

	cyng::continuation open_connection::run()
	{	
		if (!is_initialized_)
		{
			//
			//	update task state
			//
			is_initialized_ = true;

			//
			//	* store sequence - task relation
			//	* forward connection open request to device
			//	* start timer to check connection setup
			//
			//	"ipt.seq.push" can only be invoked when "req.open.connection" has generated the new sequence number

			vm_.access([this](cyng::vm& vm)->void {
				vm.run(cyng::generate_invoke("req.open.connection", number_));
				vm.run(cyng::generate_invoke("session.store.relation", cyng::invoke("ipt.seq.push"), base_.get_id()));
				vm.run(cyng::generate_invoke("stream.flush"));
				vm.run(cyng::generate_invoke("log.msg.info", "client.req.open.connection.forward, seq: ", cyng::invoke("ipt.seq.push"), ", MSISDN: ", number_));
				}
			);

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
			<< "> "
			<< vm_.tag()
			<< " - timeout after "
			<< cyng::to_str(std::chrono::system_clock::now() - start_));

		//
		//	Safe way to intentionally close this session.
		//	
		//	* set session in shutdown state
		//	* close socket
		//	* update cluster master state and
		//	* remove session from IP-T masters client_map
		//
		vm_.async_run({ 
			cyng::generate_invoke("session.state.pending"), 
			cyng::generate_invoke("ip.tcp.socket.shutdown"), 
			cyng::generate_invoke("ip.tcp.socket.close") 
		});

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
			<< "> "
			<< vm_.tag()
			<< " - is stopped after "
			<< cyng::to_str(std::chrono::system_clock::now() - start_));

	}

	//	slot 0
	cyng::continuation open_connection::process(ipt::response_type res)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< vm_.tag()
			<< " - received response ["
			<< ipt::tp_res_open_connection_policy::get_response_name(res)
			<< "]");

		return cyng::continuation::TASK_STOP;
	}
}
