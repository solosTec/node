/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "iec.h"
#include "../cache.h"
#include <smf/sml/event.h>
#include <smf/sml/obis_db.h>

#include <cyng/intrinsics/buffer.h>

namespace node
{
	iec::iec(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cyng::controller& vm
		, cache& cfg
		, std::size_t tsk)
	: base_(*btp)
		, logger_(logger)
		, cache_(cfg)
		, tsk_(tsk)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> => #"
			<< tsk_);
	}

	cyng::continuation iec::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> ");
#endif

		return cyng::continuation::TASK_CONTINUE;
	}

	void iec::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation iec::process()
	{
		CYNG_LOG_DEBUG(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> ACK");

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}


}

