/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/sml_csv_writer.h>

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

    sml_csv_writer::sml_csv_writer(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        cyng::logger logger,
        std::filesystem::path out,
        std::string prefix,
        std::string suffix,
        bool header)
        : sigs_{std::bind(&sml_csv_writer::open_response, this, std::placeholders::_1, std::placeholders::_2),  // open.response 
        std::bind(&sml_csv_writer::close_response, this), // close.response
        std::bind(&sml_csv_writer::get_profile_list_response, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6), // get.profile.list.response
        std::bind(&sml_csv_writer::get_proc_parameter_response, this), // get.proc.parameter.response
        std::bind(&sml_csv_writer::stop, this, std::placeholders::_1)   //  stop()
    }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , root_dir_(out)
        , prefix_(prefix)
        , suffix_(suffix) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"open.response", "close.response", "get.profile.list.response", "get.proc.parameter.response"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    void sml_csv_writer::stop(cyng::eod) {}
    void sml_csv_writer::open_response(cyng::buffer_t, cyng::buffer_t) {}
    void sml_csv_writer::close_response() {}
    void sml_csv_writer::get_profile_list_response(
        std::string trx,
        cyng::buffer_t server_id,
        cyng::object act_time,
        std::uint32_t status,
        cyng::obis_path_t path,
        cyng::param_map_t values) {

        // CYNG_LOG_TRACE(logger_, "[sml.csv.writer] get_profile_list_response: #" << values.size());

        auto const file_name = get_csv_filename(prefix_, suffix_, srv_id_to_str(server_id), std::chrono::system_clock::now());
        auto const file_path = root_dir_ / file_name;

        std::ofstream of(file_path.string(), std::ios::app);

        if (of.is_open()) {

            CYNG_LOG_TRACE(logger_, "[sml.csv.writer] get_profile_list_response #" << values.size() << ": " << file_path);

            auto const srv_id = to_srv_id(server_id);
            auto const meter = get_id(srv_id);

            of << "obis,value,scaler,unit,descr" << std::endl;

            of << cyng::to_string(OBIS_SERIAL_NR) << ',' << meter << ',' << ',' << ',' << "serial number" << std::endl;

            for (auto const &value : values) {
                auto const reader = cyng::make_reader(cyng::container_cast<cyng::param_map_t>(value.second));
                auto const code = reader.get("code", ""); //  obis
                auto const unit = reader.get("unit-name", "");
                // auto const val = reader.get("value", "");
                auto const scaler = reader.get<std::int8_t>("scaler", 0);
                auto const descr = reader.get("descr", "");
                of << value.first                //  obis
                   << ',' << reader.get("value") //  value
                   << ',' << +scaler             //  scaler,
                   << ',' << unit                //  unit
                                                 //  raw
                   << ',' << descr               //  descr
                   << std::endl;
            }

        } else {
            CYNG_LOG_ERROR(logger_, "[sml.csv.writer] cannot open " << file_path);
        }
    }
    void sml_csv_writer::get_proc_parameter_response() {}

    std::filesystem::path
    get_csv_filename(std::string prefix, std::string suffix, std::string server_id, std::chrono::system_clock::time_point now) {

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

} // namespace smf
