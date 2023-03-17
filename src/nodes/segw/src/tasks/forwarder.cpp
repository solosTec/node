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
			std::bind(&forwarder::connect, this, std::placeholders::_1),	//	1
			std::bind(&forwarder::disconnect, this, std::placeholders::_1),	//	2
			std::bind(&forwarder::receive, this, std::placeholders::_1),	//	3
			std::bind(&forwarder::stop, this, std::placeholders::_1)	//	4
	}
		, channel_(wp)
		, registry_(reg)
		, logger_(logger)
		, cb_(cb)
	{
        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"connect", "disconnect", "receive"});
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void forwarder::stop(cyng::eod) {}

    void forwarder::connect(std::string lmn_task_name) {

        //
        //  Add listener to LMN
        //
        if (auto sp = channel_.lock(); sp) {
            //
            //	register as listener on LMN port 1
            //
            auto const id = sp->get_id();
            CYNG_LOG_TRACE(logger_, "[redirector] #" << id << " connect to [" << lmn_task_name << "]");
            registry_.dispatch(
                lmn_task_name,
                "add-data-sink",
                //  handle dispatch errors
                std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                id);
        }
    }

    void forwarder::disconnect(std::string lmn_task_name) {
        //
        //  remove as listener from LMN
        //
        if (auto sp = channel_.lock(); sp) {
            //
            //	remove as listener from LMN
            //
            auto const id = sp->get_id();
            CYNG_LOG_TRACE(logger_, "[redirector] #" << id << " disconnect from [" << lmn_task_name << "]");
            registry_.dispatch(
                lmn_task_name,
                "remove-data-sink",
                //  handle dispatch errors
                std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2),
                cyng::make_tuple(id));
        }
    }

    void forwarder::receive(cyng::buffer_t data) {
        //
        //	forward to listener
        //
        CYNG_LOG_TRACE(logger_, "[redirector] received " << data.size() << " bytes");
        cb_(data);
    }
} // namespace smf
