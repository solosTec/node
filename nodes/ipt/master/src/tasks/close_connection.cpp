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
#include <cyng/vm/generator.h>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	close_connection::close_connection(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, bus::shared_type bus
		, cyng::controller& vm
		, bool shutdown
		, boost::uuids::uuid tag
		, std::size_t seq
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag
		, std::chrono::seconds timeout)
	: base_(*btp)
		, logger_(logger)
		, bus_(bus)
		, vm_(vm)
		, shutdown_(shutdown)
		, tag_(tag)	//	origin tag
		, seq_(seq)
		, options_(options)
		, bag_(bag)
		, timeout_(timeout)
		//, response_(ipt::tp_res_close_connection_policy::CONNECTION_CLEARING_FAILED)
		, start_(std::chrono::system_clock::now())
		, is_waiting_(false)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is running in "
			<< (shutdown_ ? "shutdown" : "operative")
			<< " mode until "
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
				, cyng::generate_invoke("session.store.relation", cyng::invoke("ipt.seq.push"), base_.get_id())

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

		CYNG_LOG_WARNING(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> timeout after "
			<< cyng::to_str(std::chrono::system_clock::now() - start_));

		if (!vm_.is_halted()) {

			//
			//	close session
			//	could crash if session is already gone
			//
			vm_.async_run({ cyng::generate_invoke("session.remove.relation", base_.get_id())
				, cyng::generate_invoke("ip.tcp.socket.shutdown")
				, cyng::generate_invoke("ip.tcp.socket.close") });
		}
		else {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> stop (session already halted) "
				<< tag_);
		}

		if (!shutdown_ && bus_->is_online())
		{
			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> send response timeout to origin "
				<< tag_);

			bus_->vm_.async_run(client_res_close_connection(tag_
				, seq_
				, false
				, options_
				, bag_));
		}

		//
		//	stop this task
		//
		return cyng::continuation::TASK_STOP;
	}

	void close_connection::stop()
	{
		//
		//	answer pending close request and
		//	send response to cluster master
		//
		//if (!shutdown_ && bus_->is_online())
		//{
		//	CYNG_LOG_TRACE(logger_, "task #"
		//		<< base_.get_id()
		//		<< " <"
		//		<< base_.get_class_name()
		//		<< "> send response "
		//		<< ipt::tp_res_close_connection_policy::get_response_name(response_)
		//		<< " to origin "
		//		<< tag_);

		//	bus_->vm_.async_run(client_res_close_connection(tag_
		//		, seq_
		//		, ipt::tp_res_close_connection_policy::is_success(response_)
		//		, options_
		//		, bag_));
		//}

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

		if (!shutdown_ && bus_->is_online())
		{
			CYNG_LOG_TRACE(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> send response timeout to origin "
				<< tag_);

			bus_->vm_.async_run(client_res_close_connection(tag_
				, seq_
				, ipt::tp_res_close_connection_policy::is_success(res)
				, options_
				, bag_));
		}

		return cyng::continuation::TASK_STOP;
	}
}

