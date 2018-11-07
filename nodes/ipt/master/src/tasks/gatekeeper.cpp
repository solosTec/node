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
#ifdef _DEBUG
				, cyng::generate_invoke("ipt.transfer.data", cyng::make_buffer({'t', 'i', 'm', 'e', 'o', 'u', 't', '\n' }))
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
		auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_);
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