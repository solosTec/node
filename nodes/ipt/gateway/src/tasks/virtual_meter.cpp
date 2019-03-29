/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "virtual_meter.h"
#include <smf/sml/srv_id_io.h>

#include <cyng/async/task/base_task.h>

namespace node
{

	virtual_meter::virtual_meter(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::buffer_t server_id
		, cyng::store::db& config_db
		, std::chrono::seconds interval)
	: base_(*btp)
		, logger_(logger)
		, server_id_(server_id)
		, config_db_(config_db)
		, interval_(interval)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< sml::from_server_id(server_id));
	}

	cyng::continuation virtual_meter::run()
	{
		//
		//	generate meter values
		//
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> generate readings");

		//
		//	restart timer
		//
		base_.suspend(interval_);
		return cyng::continuation::TASK_CONTINUE;
	}

	void virtual_meter::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}
}
