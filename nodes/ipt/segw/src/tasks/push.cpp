/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "push.h"
#include "../cache.h"
#include <smf/sml/event.h>
#include <smf/sml/obis_db.h>

#include <cyng/io/io_chrono.hpp>

namespace node
{
	push::push(cyng::async::base_task* btp
		, cyng::logging::log_ptr logger
		, cache& cfg
		, cyng::buffer_t srv_id
		, cyng::buffer_t profile
		, std::chrono::seconds interval
		, std::chrono::seconds delay
		, std::string target)
	: base_(*btp) 
		, logger_(logger)
		, cache_(cfg)
		, srv_id_(srv_id)
		, profile_(profile)
		, interval_(interval)
		, delay_(delay)
		, target_(target)
	{
		CYNG_LOG_INFO(logger_, "initialize task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> with "
			<< cyng::to_str(interval_)
			<< " interval and "
			<< cyng::to_str(delay_)
			<< " delay to ["
			<< target_	//	push target name
			<< "]");
	}

	cyng::continuation push::run()
	{
#ifdef _DEBUG
		CYNG_LOG_TRACE(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> "
			<< sml::get_profile_name(profile_));
#endif
		//
		//	recalibrate start time dependent from profile type
		//


		//
		//	start/restart timer
		//
		base_.suspend(interval_);

		//
		//	push data
		//
		//push_data();

		return cyng::continuation::TASK_CONTINUE;
	}

	void push::stop(bool shutdown)
	{
		CYNG_LOG_INFO(logger_, "task #"
			<< base_.get_id()
			<< " <"
			<< base_.get_class_name()
			<< "> is stopped");
	}

	cyng::continuation push::process()
	{

		//
		//	continue task
		//
		return cyng::continuation::TASK_CONTINUE;
	}

}

