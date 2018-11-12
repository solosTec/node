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
		, start_(std::chrono::system_clock::now())
		, is_waiting_(false)
	{
		CYNG_LOG_INFO(logger_, "task #"
		<< base_.get_id()
		<< " <"
		<< base_.get_class_name()
		<< "> is running: "
		<< tag_
		<< " until "
		<< cyng::to_str(start_ + timeout_));

	}

	cyng::continuation gatekeeper::run()
	{	
		if (!is_waiting_)
		{
			//
			//	update task state
			//
			is_waiting_ = true;

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
			<< "> timeout");

		if (!vm_.is_halted()) {

			CYNG_LOG_WARNING(logger_, "task #"
				<< base_.get_id()
				<< " <"
				<< base_.get_class_name()
				<< "> stop "
				<< tag_);

			//
			//	Safe way to intentionally close this session.
			//	
			//	* set session in shutdown state
			//	* close socket
			//	* update cluster master state and
			//	* remove session from IP-T masters client_map
			//
			vm_.async_run({ cyng::generate_invoke("session.state.pending")
#ifdef _DEBUG
				, cyng::generate_invoke("stream.serialize", "timeout")
				, cyng::generate_invoke("stream.flush")
#endif
				, cyng::generate_invoke("ip.tcp.socket.shutdown")
				, cyng::generate_invoke("ip.tcp.socket.close") });
		}


		return cyng::continuation::TASK_STOP;
	}

	void gatekeeper::stop()
	{
		//
		//	terminate task
		//
		const auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_);
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped: "
			<< tag_
			<< " after "
			<< cyng::to_str(uptime));

	}

	//	slot 0
	cyng::continuation gatekeeper::process(bool success)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< tag_
			<< " login ["
			<< (success ? "success" : "failed")
			<< "]");

		return cyng::continuation::TASK_STOP;
	}

}
