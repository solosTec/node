/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/sml_log_writer.h>

#include <smf/mbus/server_id.h>
#include <smf/obis/profile.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    sml_log_writer::sml_log_writer(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger,
        std::filesystem::path out,
        std::string prefix,
        std::string suffix)
        : sigs_{
            std::bind(&sml_log_writer::open_response, this, std::placeholders::_1, std::placeholders::_2), // open_response
            std::bind(&sml_log_writer::close_response, this), // close_response
            std::bind(&sml_log_writer::get_profile_list_response, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6), // get_profile_list_response
            std::bind(&sml_log_writer::get_proc_parameter_response, this), // get_proc_parameter_response
            std::bind(&sml_log_writer::stop, this, std::placeholders::_1) // stop
        }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , protocol_(ctl.create_channel<cyng::log>()) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"open.response", "close.response", "get.profile.list.response", "get.proc.parameter.response"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }

        protocol_.start_file_logger(out / sml_log_filename(prefix, suffix), 32UL * 1024UL * 1024UL);
    }

    void sml_log_writer::stop(cyng::eod) {
        CYNG_LOG_TRACE(logger_, "[sml.log.writer] stop");
        protocol_.stop();
    }

    void sml_log_writer::open_response(cyng::buffer_t client, cyng::buffer_t server) {
        CYNG_LOG_INFO(protocol_, "[sml.log.writer] open_response(" << client << ", " << server << ")");
    }
    void sml_log_writer::close_response() {}
    void sml_log_writer::get_profile_list_response(
        std::string trx,
        cyng::buffer_t server_id,
        cyng::object act_time,
        std::uint32_t status,
        cyng::obis_path_t path,
        cyng::prop_map_t values) {
        CYNG_LOG_TRACE(logger_, "[sml.log.writer] get_profile_list_response #" << values.size());
        CYNG_LOG_TRACE(
            protocol_,
            "[sml.log.writer] get_profile_list_response("
                << "trx: " << trx << ")");

        auto const id = srv_id_to_str(server_id);
        CYNG_LOG_INFO(
            protocol_,
            "[sml.log.writer] get_profile_list_response("
                << "server: " << id << ")");

        CYNG_LOG_INFO(
            protocol_,
            "[sml.log.writer] get_profile_list_response("
                << "time: " << act_time << ")");

        CYNG_LOG_INFO(
            protocol_,
            "[sml.log.writer] get_profile_list_response("
                << "status: " << status << ")");

        if (path.size() == 1) {
            auto const &profile = path.front();
            if (!sml::is_profile(profile)) {
                CYNG_LOG_WARNING(
                    protocol_,
                    "[sml.log.writer] get_profile_list_response("
                        << "profile " << profile << " is not supported)");
            } else {
                CYNG_LOG_INFO(
                    protocol_,
                    "[sml.log.writer] get_profile_list_response("
                        << "profile: " << profile << ")");
            }

        } else {
            CYNG_LOG_WARNING(
                protocol_,
                "[sml.log.writer] get_profile_list_response("
                    << "invalid path " << path);
        }

        CYNG_LOG_INFO(
            protocol_,
            "[sml.log.writer] get_profile_list_response("
                << "data: " << values << ")");
    }
    void sml_log_writer::get_proc_parameter_response() {}

    std::filesystem::path sml_log_filename(std::string prefix, std::string suffix) {

        std::stringstream ss;
        ss << prefix << '.' << suffix;
        return ss.str();
    }

} // namespace smf
