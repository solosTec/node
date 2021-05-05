/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/gatekeeper.h>
#include <ipt_session.h>

#include <cyng/task/channel.h>
#include <cyng/obj/util.hpp>
#include <cyng/log/record.h>

#include <iostream>

namespace smf {


	gatekeeper::gatekeeper(cyng::channel_weak wp
		, cyng::logger logger
		, std::chrono::seconds timeout
		, std::shared_ptr<ipt_session> iptsp)
	: sigs_{ 
		std::bind(&gatekeeper::timeout, this),
		std::bind(&gatekeeper::stop, this, std::placeholders::_1),
	}
	, channel_(wp)
	, logger_(logger)
	, iptsp_(iptsp)
	{
		BOOST_ASSERT(iptsp_);
		auto sp = wp.lock();
		if (sp) {
			sp->set_channel_name("timeout", 0);
			CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created - timeout is " << timeout.count() << " seconds");
			sp->suspend(timeout, "timeout", cyng::make_tuple());
		}
	}

	gatekeeper::~gatekeeper()
	{
#ifdef _DEBUG_IPT
		std::cout << "gatekeeper(~)" << std::endl;
#endif
	}

	void gatekeeper::timeout() {
		CYNG_LOG_WARNING(logger_, "[gatekeeper] timeout");
		iptsp_->stop();
		auto sp = channel_.lock();
		if (sp) sp->stop();
	}

	void gatekeeper::stop(cyng::eod)
	{
		CYNG_LOG_INFO(logger_, "[gatekeeper] stop");
	}

}


