/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <influxdb.h>
#include <smf/mbus/server_id.h>
#include <smf/obis/conv.h>
#include <tasks/sml_influx_writer.h>

#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    sml_influx_writer::sml_influx_writer(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        cyng::logger logger,
        std::string const &host,
        std::string const &service,
        std::string const &protocol,
        std::string const &cert,
        std::string const &db)
        : sigs_{std::bind(&sml_influx_writer::open_response, this, std::placeholders::_1, std::placeholders::_2), std::bind(&sml_influx_writer::close_response, this), std::bind(&sml_influx_writer::get_profile_list_response, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6), std::bind(&sml_influx_writer::get_proc_parameter_response, this), std::bind(&sml_influx_writer::stop, this, std::placeholders::_1)}
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
            sp->set_channel_name("open.response", slot++);
            sp->set_channel_name("close.response", slot++);
            sp->set_channel_name("get.profile.list.response", slot++);
            sp->set_channel_name("get.proc.parameter.response", slot++);
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    sml_influx_writer::~sml_influx_writer() {
#ifdef _DEBUG_STORE
        std::cout << "sml_influx_writer(~)" << std::endl;
#endif
    }

    void sml_influx_writer::stop(cyng::eod) {}
    void sml_influx_writer::open_response(cyng::buffer_t, cyng::buffer_t) {}
    void sml_influx_writer::close_response() {}
    void sml_influx_writer::get_profile_list_response(
        std::string trx,
        cyng::buffer_t server_id,
        cyng::object act_time,
        std::uint32_t status,
        cyng::obis_path_t path,
        cyng::param_map_t values) {

        if (path.size() == 1) {

            auto const profile = obis::to_str_vector(path, true).at(0);
            CYNG_LOG_TRACE(logger_, "[sml.influx] get_profile_list_response #" << values.size() << ": " << profile);

            send_write_stmt(profile, server_id, act_time, /*reg_period, */ status, values);

        } else {
            CYNG_LOG_WARNING(logger_, "[sml.influx] get_profile_list_response - invalid path size: " << path.size());
        }
    }
    void sml_influx_writer::get_proc_parameter_response() {}

    void sml_influx_writer::send_write_stmt(
        std::string profile,
        cyng::buffer_t server_id,
        cyng::object actTime,
        // std::uint32_t reg_period,
        std::uint32_t status,
        cyng::param_map_t const &pmap) {

        auto const srv_id = to_srv_id(server_id);

        //	example
        for (auto const &v : pmap) {
            std::stringstream ss;
            //
            //  measurement
            //
            ss << "SML";

            //
            //	tags:
            //  tags are indexd
            //
            ss << ",profile=" << profile << ",server=" << srv_id_to_str(server_id) << ",obis=" << v.first << ",area=unknown";

            //
            //   space separator
            //
            ss << ' ';

            //
            //	fields:
            //  fields are not indexed
            //
            ss << "status=" << status << ",meter=\"" << get_id(srv_id) << "\"";

            auto const readout = cyng::container_cast<cyng::param_map_t>(v.second);
            for (auto const &val : readout) {
                if (boost::algorithm::equals(val.first, "raw")) {
                    //
                    //  always string
                    //
                    ss << "," << val.first << "=\"" << cyng::io::to_plain(val.second) << "\"";
                } else if (boost::algorithm::equals(val.first, "value")) {
                    //
                    //  obis = value
                    //
                    ss << "," << v.first << "=" << influx::to_line_protocol(val.second);
                } else if (boost::algorithm::equals(val.first, "reg_period")) {
                    //  skip
                } else {
                    ss << "," << val.first << "=" << influx::to_line_protocol(val.second);
                }
            }

            //
            //	timestamp
            //
            ss << ' ' << influx::to_line_protocol(actTime) << '\n';
            auto const stmt = ss.str();

            CYNG_LOG_TRACE(logger_, "[sml.influx] write: " << stmt);
            auto const rs = influx::push_over_http(ctl_.get_ctx(), host_, service_, protocol_, cert_, db_, stmt);
            if (!rs.empty())
                CYNG_LOG_WARNING(logger_, "[sml.influx] result: " << rs);
        }
    }

} // namespace smf