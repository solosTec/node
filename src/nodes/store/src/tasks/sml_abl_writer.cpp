/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/sml_abl_writer.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    sml_abl_writer::sml_abl_writer(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger)
        : sigs_{std::bind(&sml_abl_writer::open_response, this, std::placeholders::_1, std::placeholders::_2), std::bind(&sml_abl_writer::close_response, this), std::bind(&sml_abl_writer::get_profile_list_response, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6), std::bind(&sml_abl_writer::get_proc_parameter_response, this), std::bind(&sml_abl_writer::stop, this, std::placeholders::_1)}
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger) {
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

    sml_abl_writer::~sml_abl_writer() {
#ifdef _DEBUG_STORE
        std::cout << "sml_abl_writer(~)" << std::endl;
#endif
    }

    void sml_abl_writer::stop(cyng::eod) {}
    void sml_abl_writer::open_response(cyng::buffer_t, cyng::buffer_t) {}
    void sml_abl_writer::close_response() {}
    void sml_abl_writer::get_profile_list_response(
        std::string,
        cyng::buffer_t,
        cyng::object,
        std::uint32_t,
        cyng::obis_path_t,
        cyng::param_map_t) {}
    void sml_abl_writer::get_proc_parameter_response() {}

} // namespace smf