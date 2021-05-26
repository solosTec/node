/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/forwarder.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/registry.h>

#include <fstream>
#include <iostream>

namespace smf {
    forwarder::forwarder(cyng::channel_weak wp
		, cyng::registry& reg
		, cyng::logger logger
		, cb_f cb)
	: sigs_{
			std::bind(&forwarder::start, this, std::placeholders::_1),	//	1
			std::bind(&forwarder::receive, this, std::placeholders::_1),	//	2
			std::bind(&forwarder::stop, this, std::placeholders::_1)	//	0
	}
		, channel_(wp)
		, registry_(reg)
		, logger_(logger)
		, cb_(cb)
	{
        auto sp = channel_.lock();
        if (sp) {
	        std::size_t slot{0};
            sp->set_channel_name("start", slot++);
            sp->set_channel_name("receive", slot++);
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void forwarder::stop(cyng::eod) {}

    void forwarder::start(std::string lmn_task_name) {
        //
        //	register as listener on LMN port 1
        //
        CYNG_LOG_TRACE(logger_, "[redirector] registers als listener for [" << lmn_task_name << "]");

        //	"reset-target-channels"
        registry_.dispatch(lmn_task_name, "reset-target-channels", cyng::make_tuple("redirector"));
    }

    void forwarder::receive(cyng::buffer_t data) {
        //
        //	forward to listener
        //
        CYNG_LOG_TRACE(logger_, "[redirector] received " << data.size() << " bytes");
        cb_(data);
    }
} // namespace smf
