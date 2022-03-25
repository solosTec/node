/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/iec_db_writer.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    iec_db_writer::iec_db_writer(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger)
    : sigs_{			
            std::bind(&iec_db_writer::open, this, std::placeholders::_1),
			std::bind(&iec_db_writer::store, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			std::bind(&iec_db_writer::commit, this),
			std::bind(&iec_db_writer::stop, this, std::placeholders::_1),
        }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"open", "store", "commit"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    iec_db_writer::~iec_db_writer() {}

    void iec_db_writer::stop(cyng::eod) {}
    void iec_db_writer::open(std::string id) { id_ = id; }
    void iec_db_writer::store(cyng::obis code, std::string value, std::string unit) {}
    void iec_db_writer::commit() { CYNG_LOG_TRACE(logger_, "[iec.db.writer] commit \"" << id_ << "\""); }

} // namespace smf
