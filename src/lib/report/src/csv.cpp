
#include <smf/report/csv.h>
#include <smf/report/utility.h>

#include <smf/config/schemes.h>
#include <smf/mbus/units.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/intrinsics/date.h>
#include <cyng/obj/util.hpp>

#include <fstream>
#include <iostream>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    void generate_csv(
        cyng::db::session db,
        cyng::obis profile,
        std::filesystem::path root,
        std::chrono::hours backtrack,
        std::chrono::system_clock::time_point now,
        std::string prefix) {

        switch (profile.to_uint64()) {
        case CODE_PROFILE_1_MINUTE: {
            auto const tp = cyng::utc_cast<std::chrono::system_clock::time_point>(
                cyng::make_date_from_local_time(now - backtrack).get_start_of_day());
            csv::generate_report_1_minute(db, profile, root, prefix, tp, now);
        } break;
        case CODE_PROFILE_15_MINUTE: {
            auto const start = cyng::utc_cast<std::chrono::system_clock::time_point>(
                cyng::make_date_from_local_time(now - backtrack).get_start_of_day());
            auto const end =
                cyng::utc_cast<std::chrono::system_clock::time_point>(cyng::make_date_from_local_time(now).get_end_of_day());
            csv::generate_report_15_minutes(db, profile, root, prefix, start, end);
        } break;
        case CODE_PROFILE_60_MINUTE: {
            auto const start = cyng::utc_cast<std::chrono::system_clock::time_point>(
                cyng::make_date_from_local_time(now - backtrack).get_start_of_day());
            auto const end =
                cyng::utc_cast<std::chrono::system_clock::time_point>(cyng::make_date_from_local_time(now).get_end_of_day());
            csv::generate_report_60_minutes(db, profile, root, prefix, start, end);
        } break;
        case CODE_PROFILE_24_HOUR: {
            auto const start = cyng::make_date_from_local_time(now - backtrack).get_start_of_month().to_utc_time_point();
            auto const end = cyng::make_date_from_local_time(now).get_end_of_month().to_utc_time_point();
            csv::generate_report_24_hour(db, profile, root, prefix, start, end);
        } break;
        case CODE_PROFILE_1_MONTH: {
            auto const start = cyng::make_date_from_local_time(now - backtrack).get_start_of_year().to_utc_time_point();
            auto const end = cyng::make_date_from_local_time(now).get_end_of_year().to_utc_time_point();
            csv::generate_report_1_month(db, profile, root, prefix, start, end);
        } break;
        case CODE_PROFILE_1_YEAR: {
            auto const start = cyng::make_date_from_local_time(now - backtrack).get_start_of_year().to_utc_time_point();
            auto const end = cyng::make_date_from_local_time(now).get_end_of_year().to_utc_time_point();
            csv::generate_report_1_year(db, profile, root, prefix, start, end);
        } break;

        default: break;
        }
    }

    namespace csv {
        void generate_report_1_minute(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now) {

#ifdef _DEBUG
            std::cout << "minutely reports start at " << start << std::endl;
#endif

            //
            //  generate reports about the complete time range
            //
            while (start < now) {

                start = generate_report_1_minute(db, profile, root, prefix, start, std::chrono::hours(24));
            }
        }

        std::chrono::system_clock::time_point generate_report_1_minute(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::hours span) {

            auto const end = start + span;

#ifdef _DEBUG
            std::cout << "start " << start << " => " << end << std::endl;
#endif

            //
            //  (1) select all meters of this profile that have entries in this
            // time range
            //
            auto const meters = select_meters(db, profile, start, end);

            //
            //  generate a report for each meter
            //
            for (auto const &meter : meters) {
                collect_report(db, profile, root, prefix, start, end, to_srv_id(meter));
            }

            return end;
        }

        void generate_report_15_minutes(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now) {

#ifdef _DEBUG
            std::cout << "15 min (" << profile << ") csv report period spans from " << start << " to " << now << " ("
                      << std::chrono::duration_cast<std::chrono::hours>(now - start) << ")" << std::endl;
#endif

            //
            //  generate csv reports about the complete time range
            //
            while (start < now) {

                start = generate_report_15_minutes(db, profile, root, prefix, start, std::chrono::hours(24));
            }
        }

        std::chrono::system_clock::time_point generate_report_15_minutes(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::hours span) {

            auto const end = start + span;

#ifdef _DEBUG
            std::cout << "15 min (" << profile << ") report period spans from " << start << " => " << end << " ("
                      << std::chrono::duration_cast<std::chrono::hours>(end - start) << ")" << std::endl;
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
                collect_report(db, profile, root, prefix, start, end, to_srv_id(meter));
            }

            return end;
        }

        void generate_report_60_minutes(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now) {

#ifdef _DEBUG
            std::cout << "1h (" << profile << ") report period spans from " << start << " to " << now << " ("
                      << std::chrono::duration_cast<std::chrono::hours>(now - start) << ")" << std::endl;
#endif

            //
            //  generate reports about the complete time range
            //
            while (start < now) {

                start = generate_report_60_minutes(db, profile, root, prefix, start, std::chrono::hours(24));
            }
        }

        std::chrono::system_clock::time_point generate_report_60_minutes(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::hours span) {

            auto const end = start + span;

#ifdef _DEBUG
            std::cout << "1h (" << profile << ") report period spans from " << start << " => " << end << " ("
                      << std::chrono::duration_cast<std::chrono::hours>(end - start) << ")" << std::endl;

#endif

            //
            //  (1) select all meters of this profile that have entries in this
            // time range
            //
            auto const meters = select_meters(db, profile, start, end);

            //
            //  generate a report for each meter
            //
            for (auto const &meter : meters) {
                collect_report(db, profile, root, prefix, start, end, to_srv_id(meter));
            }

            return end;
        }

        void generate_report_24_hour(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now) {
#ifdef _DEBUG
            std::cout << "daily reports start at " << start << std::endl;
#endif

            //
            //  generate reports about the complete time range
            //
            while (start < now) {

                start = generate_report_24_hour(db, profile, root, prefix, start, std::chrono::hours(24));
            }
        }

        std::chrono::system_clock::time_point generate_report_24_hour(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::hours span) {

            auto const end = start + span;

#ifdef _DEBUG
            std::cout << "start " << start << " => " << end << std::endl;
#endif

            //
            //  (1) select all meters of this profile that have entries in this
            // time range
            //
            auto const meters = select_meters(db, profile, start, end);

            //
            //  generate a report for each meter
            //
            for (auto const &meter : meters) {
                collect_report(db, profile, root, prefix, start, end, to_srv_id(meter));
            }

            return end;
        }

        void generate_report_1_month(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now) {
            //  ToDo: implement
        }
        void generate_report_1_year(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now) {
            //  ToDo: implement
        }

        void emit_report(
            std::filesystem::path root,
            std::string file_name,
            cyng::obis profile,
            std::set<cyng::obis> regs,
            std::map<std::uint64_t, std::map<cyng::obis, sml_data>> const &data) {

            auto const file_path = root / file_name;
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
                    using cyng::operator<<;
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

        void collect_report(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point end,
            srv_id_t srv_id) {

            //
            //  server id as "octet"
            //
            auto const id = to_buffer(srv_id);

            //
            //  collect data from this time range ordered by time stamp
            //
            std::map<std::uint64_t, std::map<cyng::obis, sml_data>> const data =
                collect_data_by_timestamp(db, profile, id, start, end);

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
            //
            auto const file_name = get_filename(prefix, profile, srv_id, start);
            emit_report(root, file_name, profile, regs, data);
        }
    } // namespace csv
} // namespace smf
