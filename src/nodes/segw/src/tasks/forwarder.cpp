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
        auto sp = channel_.lock();
        if (sp) {
            sp->set_channel_names({"connect", "disconnect", "receive"});
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void forwarder::stop(cyng::eod) {}

    void forwarder::connect(std::string lmn_task_name) {

        //
        //  Add listener to LMN
        //
        auto sp = channel_.lock();
        BOOST_ASSERT(sp);
        if (sp) {
            //
            //	register as listener on LMN port 1
            //
            auto const id = sp->get_id();
            CYNG_LOG_TRACE(logger_, "[redirector] #" << id << " connect to [" << lmn_task_name << "]");
            registry_.dispatch(lmn_task_name, "add-data-sink", cyng::make_tuple(id));
        }
    }

    void forwarder::disconnect(std::string lmn_task_name) {
        //
        //  remove as listener from LMN
        //
        auto sp = channel_.lock();
        BOOST_ASSERT(sp);
        if (sp) {
            //
            //	remove as listener from LMN
            //
            auto const id = sp->get_id();
            CYNG_LOG_TRACE(logger_, "[redirector] #" << id << " disconnect from [" << lmn_task_name << "]");
            registry_.dispatch(lmn_task_name, "remove-data-sink", cyng::make_tuple(id));
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
