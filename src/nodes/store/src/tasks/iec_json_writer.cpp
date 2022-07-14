#include <tasks/iec_json_writer.h>

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

    iec_json_writer::iec_json_writer(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        cyng::logger logger,
        std::filesystem::path out,
        std::string prefix,
        std::string suffix)
        : sigs_{
            std::bind(&iec_json_writer::open, this, std::placeholders::_1),
            std::bind(&iec_json_writer::store, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            std::bind(&iec_json_writer::commit, this),
            std::bind(&iec_json_writer::stop, this, std::placeholders::_1),
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

    void iec_json_writer::stop(cyng::eod) {}
    void iec_json_writer::open(std::string id) { CYNG_LOG_TRACE(logger_, "[iec.json.writer] update meter \"" << id << "\""); }
    void iec_json_writer::store(cyng::obis code, std::string value, std::string unit) {}
    void iec_json_writer::commit() { CYNG_LOG_TRACE(logger_, "[iec.json.writer] commit"); }

    std::filesystem::path
    iec_json_filename(std::string prefix, std::string suffix, std::string server_id, std::chrono::system_clock::time_point now) {

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
