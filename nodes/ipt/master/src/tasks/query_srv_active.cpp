/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "query_srv_active.h"
#include <smf/sml/srv_id_io.h>
#include <smf/sml/protocol/generator.h>
#include <cyng/vm/generator.h>

namespace node
{
	query_srv_active::query_srv_active(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, bus::shared_type bus
		, cyng::controller& vm
		, boost::uuids::uuid tag_remote
		, std::uint64_t seq_cluster		//	cluster seq
		, cyng::buffer_t const& server_id	//	server id
		, std::string user
		, std::string pwd
		, boost::uuids::uuid tag_ctx)
	: base_(*btp)
		, logger_(logger)
		, vm_(vm)
		, tag_remote_(tag_remote)
		, server_id_(server_id)
		, user_(user)
		, pwd_(pwd)
		, start_(std::chrono::system_clock::now())
		, is_waiting_(false)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is running: "
			<< sml::from_server_id(server_id));

	}

	cyng::continuation query_srv_active::run()
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
			//base_.suspend(timeout_);

			return cyng::continuation::TASK_CONTINUE;
		}

		CYNG_LOG_WARNING(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> timeout");

		return cyng::continuation::TASK_STOP;
	}

	//	slot 1
	cyng::continuation query_srv_active::process()
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


	void query_srv_active::stop()
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
			<< sml::from_server_id(server_id_)
			<< " after "
			<< uptime.count()
			<< " milliseconds");

	}

	//	slot 0
	cyng::continuation query_srv_active::process(ipt::response_type res)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< sml::from_server_id(server_id_)
			<< " received response ["
			<< "]");

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
		std::map<std::string, std::size_t> cyng::async::task<node::query_srv_active>::slot_names_({ { "shutdown", 1 } });

	}
}