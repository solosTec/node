/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/sml_json_writer.h>

#include <smf/mbus/flag_id.h>
#include <smf/mbus/server_id.h>
#include <smf/mbus/units.h>

#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <fstream>
#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    sml_json_writer::sml_json_writer(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        cyng::logger logger,
        std::filesystem::path out,
        std::string prefix,
        std::string suffix)
        : sigs_{std::bind(&sml_json_writer::open_response, this, std::placeholders::_1, std::placeholders::_2), // open.response
        std::bind(&sml_json_writer::close_response, this), // close.response
        std::bind(&sml_json_writer::get_profile_list_response, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6), // get.profile.list.response
        std::bind(&sml_json_writer::get_proc_parameter_response, this), std::bind(&sml_json_writer::stop, this, std::placeholders::_1) // get.proc.parameter.response
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

    void sml_json_writer::stop(cyng::eod) {}
    void sml_json_writer::open_response(cyng::buffer_t, cyng::buffer_t) {}
    void sml_json_writer::close_response() {}
    void sml_json_writer::get_profile_list_response(
        std::string trx,
        cyng::buffer_t server_id,
        cyng::object act_time,
        std::uint32_t status,
        cyng::obis_path_t path,
        cyng::prop_map_t values) {

        if (path.size() == 1) {
            auto const &profile = path.front();

            auto const id = srv_id_to_str(server_id);
            auto const file_name = sml_json_filename(prefix_, suffix_, id, std::chrono::system_clock::now());
            auto const file_path = root_dir_ / file_name;

            std::ofstream of(file_path.string(), std::ios::app);

            if (of.is_open()) {

                CYNG_LOG_TRACE(logger_, "[sml.json.writer] get_profile_list_response #" << values.size() << ": " << file_path);
                auto const tpl = cyng::make_tuple(
                    cyng::make_param("trx", trx),
                    cyng::make_param("serverId", id),
                    cyng::make_param("actTime", act_time),
                    cyng::make_param("status", status),
                    cyng::make_param("profile", profile),
                    cyng::make_param("data", values));
                cyng::io::serialize_json(of, tpl);

            } else {
                CYNG_LOG_ERROR(logger_, "[sml.json.writer] cannot open " << file_path);
            }
        } else {
            CYNG_LOG_WARNING(logger_, "[sml.json.writer] get_profile_list_response - invalid path size: " << path);
        }
    }
    void sml_json_writer::get_proc_parameter_response() {}

    std::filesystem::path
    sml_json_filename(std::string prefix, std::string suffix, std::string server_id, std::chrono::system_clock::time_point now) {

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
