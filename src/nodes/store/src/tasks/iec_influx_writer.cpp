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

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"open", "store", "commit"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void iec_influx_writer::stop(cyng::eod) {}
    void iec_influx_writer::open(std::string id) {
        id_ = id;
        CYNG_LOG_TRACE(logger_, "[iec.influx.writer] update meter \"" << id << "\"");
    }
    void iec_influx_writer::store(cyng::obis code, std::string value, std::string unit) {

        //
        //  Only send values that also have a physical unit.
        //
        if (unit.empty() || id_.empty()) {
            CYNG_LOG_WARNING(logger_, "[iec.influx.writer] missing server id or unit: " << id_ << ", " << unit);
        } else {

            auto const area = influx::get_area_name(id_);
            std::stringstream ss;
            //
            //  measurement
            //
            ss << "IEC";

            //
            //	tags:
            //  tags are indexd
            //
            ss << ",server=" << id_ << ",obis=" << code << ",area=" << area;

            //
            //   space separator
            //
            ss << ' ';

            //
            //	fields:
            //  fields are not indexed
            //  ToDo: Remove "meter" field because it's a duplicate of "fServer".
            //  Make sure that Grafana has no more references on it.
            //
            ss << "meter=\"" << id_ << "\"," << code << "=\"" << value << "\",unit=\"" << unit << "\",fArea=\"" << area
               << "\",fServer=\"" << id_ << "\"";
            //
            //	timestamp
            //
            ss << ' ' << influx::to_line_protocol(std::chrono::system_clock::now()) << '\n';
            auto const stmt = ss.str();

            CYNG_LOG_TRACE(logger_, "[iec.influx.writer] write: " << stmt);
            auto const rs = influx::push_over_http(ctl_.get_ctx(), host_, service_, protocol_, cert_, db_, stmt);
            if (!rs.empty()) {
                CYNG_LOG_WARNING(logger_, "[iec.influx.writer] result: " << rs);
            }
        }
    }
    void iec_influx_writer::commit() { CYNG_LOG_TRACE(logger_, "[iec.influx.writer] commit \"" << id_ << "\""); }

} // namespace smf
