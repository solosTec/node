/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <wmbus_session.h>

#include <tasks/gatekeeper.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

namespace smf {

    gatekeeper::gatekeeper(
        cyng::channel_weak wp,
        cyng::logger logger,
        std::shared_ptr<wmbus_session> wmbussp,
        std::chrono::seconds timeout)
        : sigs_{std::bind(&gatekeeper::timeout, this), std::bind(&gatekeeper::defer, this), std::bind(&gatekeeper::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , logger_(logger)
        , wmbussp_(wmbussp)
        , timeout_(timeout) {
        BOOST_ASSERT(wmbussp_);
        auto sp = channel_.lock();
        if (sp) {
            std::size_t slot{0};
            sp->set_channel_name("timeout", slot++);
            sp->set_channel_name("defer", slot++);
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    gatekeeper::~gatekeeper() {
#ifdef _DEBUG_BROKER_WMBUS
        std::cout << "gatekeeper(~)" << std::endl;
#endif
    }

    void gatekeeper::timeout() {
        CYNG_LOG_WARNING(logger_, "[gatekeeper] timeout");
        //  This will stop the gatekeeper too
        if (wmbussp_) {
            // cluster_bus_.sys_msg(cyng::severity::LEVEL_WARNING, "wM-Bus gatekeeper timeout", wmbussp_->get_remote_endpoint());
            wmbussp_->stop();
        }
    }

    void gatekeeper::defer() {
        CYNG_LOG_TRACE(logger_, "[gatekeeper] defer by " << timeout_.count() << " seconds");
        auto sp = channel_.lock();
        if (sp) {
            sp->suspend(timeout_, "timeout");
        }
    }

    void gatekeeper::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "[gatekeeper] stop"); }

} // namespace smf
