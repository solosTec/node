/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/redirector.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/registry.h>

#include <fstream>
#include <iostream>

namespace smf
{
	redirector::redirector(std::weak_ptr<cyng::channel> wp
		, cyng::registry& reg
		, cyng::logger logger
		, cb_f cb)
	: sigs_{
			std::bind(&redirector::stop, this, std::placeholders::_1),	//	0
			std::bind(&redirector::start, this, std::placeholders::_1),	//	1
			std::bind(&redirector::receive, this, std::placeholders::_1)	//	2
	}
		, channel_(wp)
		, registry_(reg)
		, logger_(logger)
		, cb_(cb)
	{
		auto sp = channel_.lock();
		if (sp) {
			sp->set_channel_name("start", 1);
			sp->set_channel_name("receive", 2);
			CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
		}
	}

	void redirector::stop(cyng::eod) {
	}

	void redirector::start(std::string lmn_task_name) {
		//
		//	register as listener on LMN port 1
		//
		CYNG_LOG_TRACE(logger_, "[redirector] registers als listener for [" << lmn_task_name << "]");

		//	"reset-target-channels"
		registry_.dispatch(lmn_task_name, "reset-target-channels", cyng::make_tuple("redirector"));

	}

	void redirector::receive(cyng::buffer_t data) {
		//
		//	forward to listener
		//
		CYNG_LOG_TRACE(logger_, "[redirector] received " << data.size() << " bytes");
		cb_(data);
	}
}

