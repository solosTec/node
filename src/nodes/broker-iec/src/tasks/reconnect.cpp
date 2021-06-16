/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/reconnect.h>

#include <cyng/log/record.h>
#include <cyng/task/channel.h>

#include <iomanip>
#include <iostream>
#include <sstream>

#include <boost/predef.h>
#include <boost/uuid/nil_generator.hpp>

#ifdef _DEBUG_BROKER_IEC
#include <cyng/io/hex_dump.hpp>
#endif

namespace smf {

    reconnect::reconnect(cyng::channel_weak wp
		, cyng::logger logger, cyng::channel_weak wp_client)
	: sigs_{
			std::bind(&reconnect::run, this),
			std::bind(&reconnect::stop, this, std::placeholders::_1),
		}
		, channel_(wp)
		, logger_(logger)
        , client_(wp_client)
	{
        auto sp = channel_.lock();
        if (sp) {
            std::size_t slot{0};
            sp->set_channel_name("run", slot++);
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    void reconnect::stop(cyng::eod) {}

    void reconnect::run() {
        auto sp = client_.lock();
        if (sp) {
            // CYNG_LOG_INFO(logger_, "[reconnect] " << sp->get_name() << " in " << reconnect_timeout_.count() << " seconds");
            sp->dispatch("connect");
        }
    }

} // namespace smf
