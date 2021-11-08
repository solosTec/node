/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/dlms_target.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    dlms_target::dlms_target(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger, ipt::bus &bus)
        : sigs_{std::bind(&dlms_target::register_target, this, std::placeholders::_1), std::bind(&dlms_target::receive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), std::bind(&dlms_target::add_writer, this, std::placeholders::_1), std::bind(&dlms_target::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , bus_(bus)
        , writer_() {
        auto sp = channel_.lock();
        if (sp) {
            sp->set_channel_names({"register", "receive", "add"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    dlms_target::~dlms_target() {
#ifdef _DEBUG_STORE
        std::cout << "dlms_target(~)" << std::endl;
#endif
    }

    void dlms_target::stop(cyng::eod) {}

    void dlms_target::register_target(std::string name) { bus_.register_target(name, channel_); }

    void dlms_target::receive(std::uint32_t channel, std::uint32_t source, cyng::buffer_t data, std::string target) {

        CYNG_LOG_TRACE(logger_, "[dlms] receive " << data.size() << " bytes");
        BOOST_ASSERT(boost::algorithm::equals(channel_.lock()->get_name(), target));
    }

    void dlms_target::add_writer(std::string name) {
        auto channels = ctl_.get_registry().lookup(name);
        if (channels.empty()) {
            CYNG_LOG_WARNING(logger_, "[dlms] writer " << name << " not found");
        } else {
            writer_.insert(writer_.end(), channels.begin(), channels.end());
            CYNG_LOG_INFO(logger_, "[dlms] add writer " << name << " #" << writer_.size());
        }
    }

} // namespace smf
