/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/sml_xml_writer.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    sml_xml_writer::sml_xml_writer(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger)
        : sigs_{std::bind(&sml_xml_writer::stop, this, std::placeholders::_1)}, channel_(wp), ctl_(ctl), logger_(logger) {
        auto sp = channel_.lock();
        if (sp) {
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    sml_xml_writer::~sml_xml_writer() {
#ifdef _DEBUG_STORE
        std::cout << "sml_xml_writer(~)" << std::endl;
#endif
    }

    void sml_xml_writer::stop(cyng::eod) {}

} // namespace smf