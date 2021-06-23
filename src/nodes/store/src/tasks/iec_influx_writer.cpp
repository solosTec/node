/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <influxdb.h>
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
    : sigs_{			
            std::bind(&iec_influx_writer::open, this, std::placeholders::_1),
			std::bind(&iec_influx_writer::store, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			std::bind(&iec_influx_writer::commit, this),
			std::bind(&iec_influx_writer::stop, this, std::placeholders::_1),
        }
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
            std::size_t slot{0};
            sp->set_channel_name("open", slot++);
            sp->set_channel_name("store", slot++);
            sp->set_channel_name("commit", slot++);
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    iec_influx_writer::~iec_influx_writer() {
#ifdef _DEBUG_STORE
        std::cout << "iec_influx_writer(~)" << std::endl;
#endif
    }

    void iec_influx_writer::stop(cyng::eod) {}
    void iec_influx_writer::open(std::string id) { id_ = id; }
    void iec_influx_writer::store(cyng::obis code, std::string value, std::string unit) {
        std::stringstream ss;
        //
        //  measurement
        //
        ss << "IEC";

        //
        //	tags:
        //  tags are indexd
        //
        ss << ",server=" << id_ << ",obis=" << code << ",area=" << influx::get_area_name(id_);

        //
        //   space separator
        //
        ss << ' ';

        //
        //	fields:
        //  fields are not indexed
        //
        ss << "meter=\"" << id_ << "\"," << code << "=" << value << ",unit=" << unit;
        //
        //	timestamp
        //
        ss << ' ' << influx::to_line_protocol(std::chrono::system_clock::now()) << '\n';
        auto const stmt = ss.str();

        CYNG_LOG_TRACE(logger_, "[iec.influx] write: " << stmt);
        auto const rs = influx::push_over_http(ctl_.get_ctx(), host_, service_, protocol_, cert_, db_, stmt);
        if (!rs.empty()) {
            CYNG_LOG_WARNING(logger_, "[iec.influx] result: " << rs);
        }
    }
    void iec_influx_writer::commit() {}

} // namespace smf
