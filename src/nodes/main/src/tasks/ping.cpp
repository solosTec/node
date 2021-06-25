/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/ping.h>

#include <cyng/log/record.h>

#include <iostream>

namespace smf {

    ping::ping(cyng::channel_weak wp
		, cyng::controller& ctl, cyng::logger logger)
	: sigs_{ 
		std::bind(&ping::stop, this, std::placeholders::_1),
	}
		, channel_(wp)
		, logger_(logger)
	{
        auto sp = channel_.lock();
        if (sp) {
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    ping::~ping() {
#ifdef _DEBUG_MAIN
        std::cout << "ping(~)" << std::endl;
#endif
    }

    void ping::stop(cyng::eod) {}

} // namespace smf
