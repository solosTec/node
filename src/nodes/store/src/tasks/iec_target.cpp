/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/iec_target.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    iec_target::iec_target(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, ipt::bus &bus)
        : sigs_{std::bind(&iec_target::register_target, this, std::placeholders::_1), std::bind(&iec_target::receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), std::bind(&iec_target::stop, this, std::placeholders::_1)},
          channel_(wp),
          ctl_(ctl),
          logger_(logger),
          bus_(bus) {
        auto sp = channel_.lock();
        if (sp) {
            sp->set_channel_name("register", 0);
            sp->set_channel_name("receive", 1);
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    iec_target::~iec_target() {
#ifdef _DEBUG_STORE
        std::cout << "iec_target(~)" << std::endl;
#endif
    }

    void iec_target::stop(cyng::eod) {
        // CYNG_LOG_WARNING(logger_, "stop network task(" << tag_ << ")");
        // if (bus_)	bus_->stop();
    }

    void iec_target::register_target(std::string name) { bus_.register_target(name, channel_); }

    void iec_target::receive(std::uint32_t channel, std::uint32_t source, cyng::buffer_t data, std::string target) {

        CYNG_LOG_TRACE(logger_, "[iec] receive " << data.size() << " bytes");
        BOOST_ASSERT(boost::algorithm::equals(channel_.lock()->get_name(), target));
    }

} // namespace smf
