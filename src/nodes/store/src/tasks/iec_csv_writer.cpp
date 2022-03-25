/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/iec_csv_writer.h>

#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/algorithm/string.hpp>

namespace smf {

    iec_csv_writer::iec_csv_writer(cyng::channel_weak wp, cyng::controller &ctl, cyng::logger logger
		, std::filesystem::path out
        , std::string prefix
        , std::string suffix
        , bool header)
    : sigs_{			
            std::bind(&iec_csv_writer::open, this, std::placeholders::_1),
			std::bind(&iec_csv_writer::store, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			std::bind(&iec_csv_writer::commit, this),
			std::bind(&iec_csv_writer::stop, this, std::placeholders::_1),
        }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)		
        , out_(out)
		, ostream_()
        , id_()
 {
        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"open", "store", "commit"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] created");
        }
    }

    iec_csv_writer::~iec_csv_writer() {}

    void iec_csv_writer::stop(cyng::eod) {}

    void iec_csv_writer::open(std::string meter) {

        auto const now = std::chrono::system_clock::now();
        std::time_t const tt = std::chrono::system_clock::to_time_t(now);
        struct tm tm = {0};
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
        ::gmtime_s(&tm, &tt);
#else
        ::gmtime_r(&tt, &tm);
#endif

        //
        //	make a file name
        //
        std::stringstream ss;
        ss << std::put_time(&tm, "%FT%H-%M_") << meter << ".csv";

        std::filesystem::path path = out_ / ss.str();

        if (ostream_.is_open())
            ostream_.close();

        ostream_.open(path.string());
        if (ostream_.is_open()) {
            CYNG_LOG_INFO(logger_, "[iec.csv.writer] open " << path);
            ostream_ << "obis,value,unit" << std::endl;
        } else {
            CYNG_LOG_WARNING(logger_, "[iec.csv.writer] open " << path << " failed");
        }

        //  for logging purposes
        id_ = meter;
    }

    void iec_csv_writer::store(cyng::obis code, std::string value, std::string unit) {

        if (ostream_.is_open()) {
            try {
                //  note: id_ is empty at this time
                // CYNG_LOG_TRACE(logger_, "[iec.csv.writer] store " << id_ << ": " << value << " " << unit);
                ostream_ << code << ',' << value << ',' << unit << std::endl;
            } catch (std::exception const &ex) {
                CYNG_LOG_WARNING(logger_, "[iec.csv.writer] store failed: " << ex.what());
            }
        }
    }

    void iec_csv_writer::commit() {
        if (ostream_.is_open()) {
            CYNG_LOG_INFO(logger_, "[iec.csv.writer] commit " << id_);
            ostream_.close();
        } else {
            CYNG_LOG_INFO(logger_, "[iec.csv.writer] commit " << id_ << " failed");
        }
    }

} // namespace smf
