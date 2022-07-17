
#include <smf/report/report.h>

#include <smf/config/schemes.h>
#include <smf/mbus/units.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/db/storage.h>
#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/sql/dsl.h>
#include <cyng/sql/dsl/placeholder.h>
#include <cyng/sql/sql.hpp>
#include <cyng/sys/clock.h>
#include <cyng/task/channel.h>

#include <fstream>
#include <iostream>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    void generate_report(
        cyng::db::session db,
        cyng::obis profile,
        std::filesystem::path root,
        std::chrono::hours backtrack,
        std::chrono::system_clock::time_point now) {

        switch (profile.to_uint64()) {
        case CODE_PROFILE_1_MINUTE: generate_report_1_minute(db, cyng::sys::get_start_of_day(now - backtrack), now); break;
        case CODE_PROFILE_15_MINUTE: generate_report_15_minutes(db, cyng::sys::get_start_of_day(now - backtrack), now); break;
        case CODE_PROFILE_60_MINUTE: generate_report_60_minutes(db, cyng::sys::get_start_of_day(now - backtrack), now); break;
        case CODE_PROFILE_24_HOUR: generate_report_24_hour(db, cyng::sys::get_start_of_day(now - backtrack), now); break;
        case CODE_PROFILE_1_MONTH: generate_report_1_month(db, cyng::sys::get_start_of_month(now - backtrack), now); break;
        case CODE_PROFILE_1_YEAR: generate_report_1_year(db, cyng::sys::get_start_of_year(now - backtrack), now); break;

        default: break;
        }
    }

    void generate_report_1_minute(
        cyng::db::session db,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point now) {
        auto constexpr profile = CODE_PROFILE_1_MINUTE;
    }

    void generate_report_15_minutes(
        cyng::db::session db,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point now) {

#ifdef _DEBUG
        std::cout << "15 min reports start at " << start << std::endl;
#endif

        //
        //  generate reports about the complete time range
        //
        while (start < now) {

            start = generate_report_15_minutes(db, start, std::chrono::hours(24));
        }
    }

    std::chrono::system_clock::time_point
    generate_report_15_minutes(cyng::db::session db, std::chrono::system_clock::time_point start, std::chrono::hours span) {

        auto constexpr profile = OBIS_PROFILE_15_MINUTE;
        auto const end = start + span;

#ifdef _DEBUG
        std::cout << "start " << start << " => " << end << std::endl;
#endif

        //
        //  (1) select all meters of this profile that have entries in this
        // time range
        //
        auto const meters = select_meters(db, profile, start, end);

#ifdef _DEBUG
        if (!meters.empty()) {
            std::cout << profile << " has " << meters.size() << " meters: ";
            for (auto const &meter : meters) {
                using cyng::operator<<;
                std::cout << meter << " ";
            }
            std::cout << std::endl;
        }
#endif

        //
        //  generate a report for each meter
        //
        for (auto const &meter : meters) {
            generate_report_15_minutes(db, start, end, to_srv_id(meter));
        }

        return end;
    }

    void generate_report_15_minutes(
        cyng::db::session db,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end,
        srv_id_t srv_id) {

        auto constexpr profile = OBIS_PROFILE_15_MINUTE;
        auto const id = to_buffer(srv_id);

        //
        //  collect data of a full day
        //
        std::map<std::uint64_t, std::map<cyng::obis, sml_data>> data;

        auto const ms = config::get_table_sml_readout();
        std::string const sql =
            "SELECT tag, gen, meterID, profile, trx, status, datetime(actTime), datetime(received) FROM TSMLreadout WHERE profile = ? AND meterID = ? AND received BETWEEN julianday(?) AND julianday(?) ORDER BY actTime";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(profile), 0); //	profile
            stmt->push(cyng::make_object(id), 9);      //	meterID
            stmt->push(cyng::make_object(start), 0);   //	start time
            stmt->push(cyng::make_object(end), 0);     //	end time

            while (auto res = stmt->get_result()) {
                auto const rec = cyng::to_record(ms, res);
                auto const tag = rec.value("tag", boost::uuids::nil_uuid());
                BOOST_ASSERT(!tag.is_nil());

                //
                //  select readout data
                //
                //  std::map<cyng::obis, sml_data>
                auto const readouts = select_readout_data(db, tag);

                //
                //  calculate the time slot
                //
                auto const act_time = rec.value("actTime", start);
                auto const slot = sml::to_index(act_time, profile);
                auto set_time = sml::to_time_point(slot, profile);

#ifdef _DEBUG
                using cyng::operator<<;
                std::cout << to_string(srv_id) << " - slot: #" << slot << " (" << set_time
                          << "), actTime: " << rec.value("actTime", start) << " with " << readouts.size() << " readouts"
                          << std::endl;
#endif

                data.emplace(slot, readouts);
            }
        }

        //
        // collect all used profiles
        //
        auto const regs = collect_profiles(data);
#ifdef _DEBUG
        std::cout << to_string(srv_id) << " has " << regs.size() << " registers: ";
        for (auto const &reg : regs) {
            std::cout << reg << " ";
        }
        std::cout << std::endl;
#endif

        //
        // print report
        // data
        auto const file_name = get_filename(profile, srv_id, start);
        emit_report(file_name, profile, regs, data);
    }

    void generate_report_60_minutes(
        cyng::db::session db,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point now) {
        auto constexpr profile = CODE_PROFILE_60_MINUTE;
    }

    void generate_report_24_hour(
        cyng::db::session db,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point now) {
        auto constexpr profile = CODE_PROFILE_24_HOUR;
    }
    void generate_report_1_month(
        cyng::db::session db,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point now) {
        auto constexpr profile = CODE_PROFILE_1_MONTH;
    }
    void generate_report_1_year(
        cyng::db::session db,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point now) {
        auto constexpr profile = CODE_PROFILE_1_YEAR;
    }

    std::vector<cyng::buffer_t> select_meters(
        cyng::db::session db,
        cyng::obis profile,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) {
        std::string const sql =
            "SELECT DISTINCT meterID FROM TSMLreadout WHERE profile = ? AND received BETWEEN julianday(?) AND julianday(?)";
        auto stmt = db.create_statement();
        std::vector<cyng::buffer_t> meters;
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(profile), 0); //	profile
            stmt->push(cyng::make_object(start), 0);   //	start time
            stmt->push(cyng::make_object(end), 0);     //	end time
            while (auto res = stmt->get_result()) {
                auto const meter = cyng::to_buffer(res->get(1, cyng::TC_BUFFER, 9));
                BOOST_ASSERT(!meter.empty());
                if (!meter.empty()) {
                    meters.push_back(meter);
                }
            }
        }
        return meters;
    }

    std::map<cyng::obis, sml_data> select_readout_data(cyng::db::session db, boost::uuids::uuid tag) {

        // std::vector<sml_data> readout;
        std::map<cyng::obis, sml_data> readout;

        auto const ms = config::get_table_sml_readout_data();
        std::string const sql = "SELECT tag, register, gen, reading, type, scaler, unit FROM TSMLreadoutData WHERE tag = ?";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(tag), 0); //	tag
            while (auto res = stmt->get_result()) {
                auto const rec = cyng::to_record(ms, res);
#ifdef _DEBUG
                using cyng::operator<<;
                // std::cout << rec.to_tuple() << std::endl;
#endif
                auto const code = rec.value<std::uint16_t>("type", cyng::TC_STRING);
                auto const val = rec.value("reading", "");
                auto const scaler = rec.value<std::int8_t>("scaler", 0);
                auto const reg = rec.value("register", OBIS_DATA_COLLECTOR_REGISTER);
                auto const unit = rec.value<std::uint8_t>("unit", 0u);

                // readout.emplace_back(reg, code, scaler, unit, val);
                auto const pos = readout.emplace(
                    std::piecewise_construct, std::forward_as_tuple(reg), std::forward_as_tuple(code, scaler, unit, val));
#ifdef _DEBUG
                std::cout << tag << ": " << pos.first->second.restore() << ", value: " << val << ", scaler: " << +scaler
                          << std::endl;
#endif
            }
        }

        return readout;
    }

    std::set<cyng::obis> collect_profiles(std::map<std::uint64_t, std::map<cyng::obis, sml_data>> const &data) {
        std::set<cyng::obis> regs;
        for (auto const d : data) {
            for (auto const &v : d.second) {
                regs.insert(v.first);
            }
        }
        return regs;
    }

    std::string get_filename(cyng::obis profile, srv_id_t srv_id, std::chrono::system_clock::time_point start) {

        auto tt = std::chrono::system_clock::to_time_t(start);
#ifdef _MSC_VER
        struct tm tm;
        _gmtime64_s(&tm, &tt);
#else
        auto tm = *std::gmtime(&tt);
#endif

        std::stringstream ss;
        ss << "report-" << get_prefix(profile) << "-" << to_string(srv_id) << '_' << std::put_time(&tm, "%Y%m%dT%H%M") << ".csv";
        return ss.str();
    }

    std::string get_prefix(cyng::obis profile) {
        switch (profile.to_uint64()) {
        case CODE_PROFILE_1_MINUTE: return "1-min";
        case CODE_PROFILE_15_MINUTE: return "15-min";
        case CODE_PROFILE_60_MINUTE: return "1-h";
        case CODE_PROFILE_24_HOUR: return "1-d";
        case CODE_PROFILE_1_MONTH: return "1-mon";
        case CODE_PROFILE_1_YEAR: return "1-y";
        default: break;
        }
        return to_string(profile);
    }

    void emit_report(
        std::string file_name,
        cyng::obis profile,
        std::set<cyng::obis> regs,
        std::map<std::uint64_t, std::map<cyng::obis, sml_data>> const &data) {
        // auto const file_path = root_dir_ / file_name;
        auto const file_path = std::filesystem::current_path() / file_name;
        std::ofstream of(file_path.string(), std::ios::trunc);

        if (of.is_open()) {
            //
            //  header
            //
            of << "time,";

            bool init = false;
            for (auto const &reg : regs) {
                if (init) {
                    of << ",";
                } else {
                    init = true;
                }
                of << reg;
            }
            of << std::endl;

            //
            //  data
            //
            for (auto const &set : data) {
                //
                //  print time slot
                //
                auto set_time = sml::to_time_point(set.first, profile);
                of << set_time << ",";

                //
                //  metering data of this time slot
                //
                for (auto const &reg : regs) {
                    auto const pos = set.second.find(reg);
                    if (pos != set.second.end()) {
                        of << pos->second.reading_ << " " << mbus::get_name(pos->second.unit_);
                    }
                    of << ",";
                }
                of << std::endl;
            }
        }
    }

} // namespace smf
