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

namespace node
{
	gatekeeper::gatekeeper(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, boost::uuids::uuid tag
		, std::chrono::seconds timeout
		, cyng::vector_t && prg)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, tag_(tag)
		, timeout_(timeout)
		, prg_(std::move(prg))
		, start_(std::chrono::system_clock::now())
		, is_waiting_(false)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< tag_
			<< " is running until "
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
			<< "> "
			<< tag_
			<< " timeout");

		if (!vm_.is_halted()) {

			//
			//	Safe way to intentionally close this session.
			//	
			//	* set session in shutdown state
			//	* close socket
			//	* update cluster master state and
			//	* remove session from IP-T masters client_map
			//
			vm_.async_run({ cyng::generate_invoke("session.state.pending")
				, prg_
				, cyng::generate_invoke("stream.flush")
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
			<< "> "
			<< tag_
			<< " stopped after "
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
			<< " received response ["
			<< (success ? "success" : "failed")
			<< "]");

		return cyng::continuation::TASK_STOP;
	}

	//	slot 1
	cyng::continuation gatekeeper::process()
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> session closed");

		//
		//	session already stopped
		//
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
		std::map<std::string, std::size_t> cyng::async::task<node::gatekeeper>::slot_names_({ { "shutdown", 1 } });

	}
}