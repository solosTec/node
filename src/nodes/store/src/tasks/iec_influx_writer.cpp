/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/iec_influx_writer.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    iec_influx_writer::iec_influx_writer(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        cyng::logger logger,
        std::string const &host,
        std::string const &service,
        std::string const &protocol,
        std::string const &cert,
        std::string const &db)
        : sigs_{std::bind(&iec_influx_writer::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , host_(host)
        , service_(service)
        , protocol_(protocol)
        , cert_(cert)
        , db_(db) {
        auto sp = channel_.lock();
        if (sp) {
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    iec_influx_writer::~iec_influx_writer() {
#ifdef _DEBUG_STORE
        std::cout << "iec_influx_writer(~)" << std::endl;
#endif
    }

    void iec_influx_writer::stop(cyng::eod) {}

} // namespace smf
