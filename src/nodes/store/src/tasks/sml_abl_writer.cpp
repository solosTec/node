/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/sml_abl_writer.h>

#include <smf/mbus/flag_id.h>
#include <smf/mbus/server_id.h>
#include <smf/mbus/units.h>
#include <smf/obis/defs.h>

#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <fstream>
#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    sml_abl_writer::sml_abl_writer(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        cyng::logger logger,
        std::filesystem::path out,
        std::string prefix,
        std::string suffix,
        bool eol_dos)
        : sigs_{std::bind(&sml_abl_writer::open_response, this, std::placeholders::_1, std::placeholders::_2), // open.response 
        std::bind(&sml_abl_writer::close_response, this), // close.response
        std::bind(&sml_abl_writer::get_profile_list_response, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6), // get.profile.list.response
        std::bind(&sml_abl_writer::get_proc_parameter_response, this), std::bind(&sml_abl_writer::stop, this, std::placeholders::_1) // get.proc.parameter.response
    }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , root_dir_(out)
        , prefix_(prefix)
        , suffix_(suffix)
        , eol_(eol_dos ? "\r\n" : "\n") {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"open.response", "close.response", "get.profile.list.response", "get.proc.parameter.response"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void sml_abl_writer::stop(cyng::eod) {}
    void sml_abl_writer::open_response(cyng::buffer_t, cyng::buffer_t) {}
    void sml_abl_writer::close_response() {}
    void sml_abl_writer::get_profile_list_response(
        std::string trx,
        cyng::buffer_t server_id,
        cyng::object act_time,
        std::uint32_t status,
        cyng::obis_path_t path,
        cyng::prop_map_t values) {

        auto const file_name = get_abl_filename(prefix_, suffix_, srv_id_to_str(server_id, true), std::chrono::system_clock::now());
        auto const file_path = root_dir_ / file_name;

        std::ofstream of(file_path.string(), std::ios::app | std::ios::binary);

        if (of.is_open()) {

            CYNG_LOG_TRACE(logger_, "[sml.abl.writer] get_profile_list_response #" << values.size() << ": " << file_path);

            //  get a std::chrono::system_clock::time_point
            auto const at = get_act_time(act_time);

            emit_abl_header(of, server_id, at, eol_);

            //	complete data set
            for (auto const &value : values) {
                // BOOST_ASSERT(!pmap.empty());
                auto const reader = cyng::make_reader(cyng::container_cast<cyng::param_map_t>(value.second));
                auto const code = reader.get("key", cyng::obis());
                if (code != OBIS_SERIAL_NR && code != OBIS_SERIAL_NR_SECOND && code[cyng::obis::VG_MEDIUM] < 129 &&
                    code[cyng::obis::VG_STORAGE] != 0) {

                    auto const value = reader.get("value");
                    auto const unit = mbus::to_unit(reader.get<std::uint8_t>("unit", 0));
                    if (unit != smf::mbus::unit::COUNT) {
                        auto const unit_name = reader.get("unit.name", "");
                        emit_abl_value(of, code, value, "*" + unit_name, eol_);
                    } else {
                        emit_abl_value(of, code, value, "", eol_);
                    }
                }
            }

            //
            //  raw data
            //
            of << "Raw(0)" << eol_;

            //
            //  end of file
            //
            of << '!' << eol_;

        } else {
            CYNG_LOG_ERROR(logger_, "[sml.abl.writer] cannot open " << file_path);
        }
    }
    void sml_abl_writer::get_proc_parameter_response() {}

    std::filesystem::path get_abl_filename(
        std::string prefix,
        std::string suffix,
        // std::string gw,
        std::string server_id,
        std::chrono::system_clock::time_point now) {
        //	example: push_20181218T191428--01-a815-90870704-01-02.abl
        //	prefix datetime -- server ID . suffix

        auto tt = std::chrono::system_clock::to_time_t(now);
#ifdef _MSC_VER
        struct tm tm;
        _gmtime64_s(&tm, &tt);
#else
        auto tm = *std::gmtime(&tt);
#endif

        std::stringstream ss;
        ss << prefix << server_id << '_' << std::put_time(&tm, "%Y%m%dT%H%M%S") << '.' << suffix;
        return ss.str();
    }

    std::chrono::system_clock::time_point get_act_time(cyng::object obj) {
        return (obj.tag() == cyng::TC_TIME_POINT) ? cyng::value_cast(obj, std::chrono::system_clock::now())
                                                  : std::chrono::system_clock::now();
    }

    void emit_abl_header(std::ofstream &os, cyng::buffer_t const &srv, std::chrono::system_clock::time_point act, std::string eol) {

        auto const srv_id = to_srv_id(srv);
        auto const manufacturer = mbus::decode(get_manufacturer_code(srv_id));

        auto tt = std::chrono::system_clock::to_time_t(act);
#ifdef _MSC_VER
        struct tm tm;
        _gmtime64_s(&tm, &tt);
#else
        auto tm = *std::gmtime(&tt);
#endif

        os << "[HEADER]" << eol                               //    force windows format
           << "PROT = 0" << eol                               //	force windows format
           << "MAN1 = "                                       //    MAN1 = manufacturer
           << manufacturer                                    //    manufacturer
           << eol                                             //	force windows format
           << "ZNR1 = "                                       //    example: 03685319
           << get_meter_id(srv_id)                            //    meter id
           << eol                                             //	force windows format
           << "DATE = "                                       //    18.12.18 UTC"
           << std::put_time(&tm, "%y.%m.%d") << " UTC" << eol //	force windows format
           << "TIME = "                                       //    11.13.38 UTC"
           << std::put_time(&tm, "%H:%M:%S") << " UTC"        //    UTC
           << eol                                             //	force windows format
           << eol                                             //	force windows format
           << "[DATA]" << eol                                 //	force windows format
           << manufacturer << eol                             //	force windows format
           << OBIS_SERIAL_NR                                  //    OBIS
           << '(' << get_meter_id(srv_id) << ")"              //    meter id again
           << eol                                             //	force windows format
            ;
    }

    void emit_abl_value(std::ofstream &os, cyng::obis const &code, cyng::object const &val, std::uint8_t unit, std::string eol) {}

    void emit_abl_value(std::ofstream &os, cyng::obis const &code, cyng::object const &val, std::string unit, std::string eol) {
        os << to_string(code) << '(' << val << unit << ')' << eol //	force windows format
           << std::flush;
    }
} // namespace smf
