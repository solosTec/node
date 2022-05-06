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
    CP210x::CP210x(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger)
        : sigs_{
            std::bind(&CP210x::receive, this, std::placeholders::_1),   // receive
            std::bind(&CP210x::reset_target_channels, this),	//	1 - "reset-data-sinks"
            std::bind(&CP210x::add_target_channel, this, std::placeholders::_1),	//	2 - "add-data-sink"
            std::bind(&CP210x::stop, this, std::placeholders::_1)
    }
        , channel_(wp)
        , logger_(logger)
        , ctl_(ctl)
        , parser_([this](hci::msg const &msg) { this->put(msg); })
        , targets_() {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"receive", "reset-data-sinks", "add-data-sink"});
            CYNG_LOG_TRACE(logger_, "task [" << sp->get_name() << "] created");
        }

        CYNG_LOG_INFO(logger_, "[CP210x] ready");
    }

    void CP210x::stop(cyng::eod) { CYNG_LOG_INFO(logger_, "CP210x stopped"); }

    void CP210x::put(hci::msg const &msg) {
        CYNG_LOG_DEBUG(logger_, "[CP210x] hci message: " << msg);
        for (auto target : targets_) {
            ctl_.get_registry().dispatch(target, "receive", msg.get_payload());
        }
    }

    void CP210x::receive(cyng::buffer_t data) { parser_.read(std::begin(data), std::end(data)); }

    void CP210x::reset_target_channels() { targets_.clear(); }
    void CP210x::add_target_channel(std::string name) { targets_.insert(name); }

} // namespace smf
