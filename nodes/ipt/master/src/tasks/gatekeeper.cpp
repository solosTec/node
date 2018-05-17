/*
* The MIT License (MIT)
*
* Copyright (c) 2018 Sylko Olzscher
*
*/

#include "gatekeeper.h"
#include <smf/cluster/generator.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/io/serializer.h>
#include <cyng/vm/generator.h>
#include <cyng/factory/chrono_factory.h>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	gatekeeper::gatekeeper(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, boost::uuids::uuid tag
		, std::chrono::seconds timeout)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, tag_(tag)
		, timeout_(timeout)
		, response_(ipt::ctrl_res_login_public_policy::ACCOUNT_LOCKED)
		, start_(std::chrono::system_clock::now())
	{
		CYNG_LOG_INFO(logger_, "task #"
		<< base_.get_id()
		<< " <"
		<< base_.get_class_name()
		<< "> is running: "
		<< tag_);

	}

	void gatekeeper::run()
	{	
		//
		//	start monitor
		//
		base_.suspend(timeout_);
	}

	void gatekeeper::stop()
	{
		if (response_ == ipt::ctrl_res_login_public_policy::ACCOUNT_LOCKED)
		{
			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> stop "
				<< tag_);

			//
			//	no login response received - stop client
			//
			vm_.async_run(cyng::generate_invoke("ip.tcp.socket.shutdown"));
			vm_.async_run(cyng::generate_invoke("ip.tcp.socket.close"));

		}

		//
		//	terminate task
		//
		auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_);
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped: "
			<< tag_
			<< " after "
			<< uptime.count()
			<< " milliseconds");

	}

	//	slot 0
	cyng::continuation gatekeeper::process(ipt::response_type res)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< tag_
			<< " received response ["
			<< ipt::ctrl_res_login_public_policy::get_response_name(res)
			<< "]");

		response_ = res;
		return cyng::continuation::TASK_STOP;
	}

}
