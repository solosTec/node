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

    ping::ping(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, db &cache, std::function<void()> trigger)
        : sigs_{std::bind(&ping::update, this), std::bind(&ping::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , logger_(logger)
        , cache_(cache)
        , trigger_(trigger) {
        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"update"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    ping::~ping() {}

    void ping::update() {
        if (auto sp = channel_.lock(); sp) {

            trigger_();
            //  repeat
            sp->suspend(std::chrono::minutes(1), "update");
        }
    }

    void ping::stop(cyng::eod) {}

} // namespace smf
