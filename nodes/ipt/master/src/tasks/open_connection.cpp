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
#include <boost/uuid/random_generator.hpp>

namespace node
{
	open_connection::open_connection(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, bus::shared_type bus
		, cyng::controller& vm
		, boost::uuids::uuid tag
		, std::size_t seq
		, std::string number
		, cyng::param_map_t const& options
		, cyng::param_map_t const& bag
		, std::chrono::seconds timeout)
	: base_(*btp)
		, logger_(logger)
		, bus_(bus)
		, vm_(vm)
		, tag_(tag)	//	origin tag
		, seq_(seq)
		, number_(number)
		, options_(options)
		, bag_(bag)
		, timeout_(timeout)
		, response_(ipt::tp_res_open_connection_policy::UNREACHABLE)
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

			vm_	.async_run(cyng::generate_invoke("req.open.connection", number_))
				.async_run(cyng::generate_invoke("session.store.relation", cyng::invoke("ipt.seq.push"), base_.get_id()))
				.async_run(cyng::generate_invoke("stream.flush"))
				.async_run(cyng::generate_invoke("log.msg.info", "client.req.open.connection.forward", cyng::invoke("ipt.seq.push"), number_))
				;

			//
			//	start monitor
			//
			base_.suspend(timeout_);

			return cyng::continuation::TASK_CONTINUE;
		}

		if (!vm_.is_halted()) {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> timeout after "
				<< cyng::to_str(std::chrono::system_clock::now() - start_));

			//
			//	close session
			//	could crash if session is already gone
			//
			vm_.async_run(cyng::generate_invoke("ip.tcp.socket.shutdown"));
			vm_.async_run(cyng::generate_invoke("ip.tcp.socket.close"));
		}
		else {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> stop (session already halted) "
				<< tag_);
		}

		return cyng::continuation::TASK_STOP;
	}

	void open_connection::stop()
	{
		if (ipt::tp_res_open_connection_policy::is_success(response_))
		{
			auto dom = cyng::make_reader(options_);
			const auto local_connect = cyng::value_cast(dom.get("local-connect"), false);
			if (local_connect)
			{
				auto origin_tag = cyng::value_cast(dom.get("origin-tag"), boost::uuids::nil_uuid());
				auto remote_tag = cyng::value_cast(dom.get("remote-tag"), boost::uuids::nil_uuid());
				BOOST_ASSERT(origin_tag == tag_);	//	this station got the call
				BOOST_ASSERT(origin_tag != remote_tag);

				CYNG_LOG_DEBUG(logger_, "task #"
					<< base_.get_id()
					<< " <"
					<< base_.get_class_name()
					<< "> prepare local connection "
					<< origin_tag
					<< " <==> "
					<< remote_tag);

			}
		}

		//
		//	send response to cluster master
		//
		if (bus_->is_online()) {
			bus_->vm_.async_run(client_res_open_connection(tag_
				, seq_
				, ipt::tp_res_open_connection_policy::is_success(response_)
				, options_
				, bag_));
		}

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
	cyng::continuation open_connection::process(ipt::response_type res)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> received response ["
			<< ipt::tp_res_open_connection_policy::get_response_name(res)
			<< "]");

		response_ = res;
		return cyng::continuation::TASK_STOP;
	}

	//	slot 1
	cyng::continuation open_connection::process()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> session closed");

		return cyng::continuation::TASK_STOP;
	}

}

#include <cyng/async/task/task.hpp>

namespace cyng {
	namespace async {

		//
		//	initialize static slot names
		//
		template <>
		std::map<std::string, std::size_t> cyng::async::task<node::open_connection>::slot_names_({ { "shutdown", 1 } });

	}
}
