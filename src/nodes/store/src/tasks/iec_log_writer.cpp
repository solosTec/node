/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/iec_log_writer.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    iec_log_writer::iec_log_writer(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger)
        : sigs_{std::bind(&iec_log_writer::stop, this, std::placeholders::_1)}, channel_(wp), ctl_(ctl), logger_(logger) {
        auto sp = channel_.lock();
        if (sp) {
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    iec_log_writer::~iec_log_writer() {
#ifdef _DEBUG_STORE
        std::cout << "iec_target(~)" << std::endl;
#endif
    }

    void iec_log_writer::stop(cyng::eod) {}

} // namespace smf
