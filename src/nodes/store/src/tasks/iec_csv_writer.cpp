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
        , root_dir_(out)
        , prefix_(prefix)
        , suffix_(suffix)
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

        //
        //	make a file name
        //
        auto const now = std::chrono::system_clock::now();
        auto const file_name = iec_csv_filename(prefix_, suffix_, meter, now);
        std::filesystem::path path = root_dir_ / file_name;

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

    std::filesystem::path
    iec_csv_filename(std::string prefix, std::string suffix, std::string server_id, std::chrono::system_clock::time_point now) {
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
