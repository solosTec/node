/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <tasks/CP210x.h>

#include <cyng/log/record.h>
#include <cyng/task/controller.h>

namespace smf {
    CP210x::CP210x(std::weak_ptr<cyng::channel> wp, cyng::controller &ctl, cyng::logger logger)
        : sigs_{std::bind(&CP210x::stop, this, std::placeholders::_1), std::bind(&CP210x::receive, this, std::placeholders::_1), std::bind(&CP210x::reset_target_channels, this, std::placeholders::_1)}
        , channel_(wp)
        , logger_(logger)
        , ctl_(ctl)
        , parser_([this](hci::msg const &msg) { this->put(msg); })
        , targets_() {
        auto sp = channel_.lock();
        if (sp) {
            sp->set_channel_name("receive", 1);
            sp->set_channel_name("reset-data-sinks", 2);
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }

        CYNG_LOG_INFO(logger_, "CP210x ready");
    }

    void CP210x::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "CP210x stopped"); }

    void CP210x::put(hci::msg const &msg) {
        CYNG_LOG_DEBUG(logger_, "CP210x received " << msg);
        for (auto target : targets_) {
            target->dispatch("receive", cyng::make_tuple(msg.get_payload()));
        }
    }

    void CP210x::receive(cyng::buffer_t data) { parser_.read(std::begin(data), std::end(data)); }

    void CP210x::reset_target_channels(std::string name) {
        targets_ = ctl_.get_registry().lookup(name);
        CYNG_LOG_INFO(logger_, "[CP210x] has " << targets_.size() << " x target(s) " << name);
    }

} // namespace smf
