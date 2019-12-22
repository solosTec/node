/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include "obislog.h"
#include "../bridge.h"
#include <smf/sml/event.h>
#include <smf/sml/obis_db.h>

namespace node
{
	obislog::obislog(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, bridge& com
		, std::chrono::minutes interval_time)
	: base_(*btp) 
		, logger_(logger)
		, bridge_(com)
		, interval_time_(interval_time)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
	}

	cyng::continuation obislog::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< ">");
#endif
		//
		//	write cyclic log entry
		//
		bridge_.generate_op_log(sml::OBIS_CODE_PEER_OBISLOG	//	81 81 00 00 00 01 
			, sml::LOG_CODE_10	//	0x00800000 - Timer, Zyklischer Logbucheintrag
			, ""	//	target
			, 0		//	nr
			, "");	//	description

		//
		//	start/restart timer
		//
		base_.suspend(interval_time_);

		return cyng::continuation::TASK_CONTINUE;
	}

	void obislog::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation obislog::process()
	{

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

}

