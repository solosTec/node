
#include <smf/report/gap.h>

#include <smf/config/schemes.h>
#include <smf/mbus/server_id.h>
#include <smf/mbus/units.h>
#include <smf/obis/conv.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#include <cyng/sys/clock.h>

#include <fstream>
#include <iostream>

namespace smf {

    void generate_gap(
        cyng::db::session db,
        cyng::obis profile,
        std::filesystem::path root,
        std::chrono::hours backtrack,
        std::chrono::system_clock::time_point now) {

        //
        //	start transaction
        //
        cyng::db::transaction trx(db);

        switch (profile.to_uint64()) {
        case CODE_PROFILE_1_MINUTE:
            gap::generate_report_1_minute(
                db,
                profile,
                root,
                cyng::sys::get_start_of_day(now - backtrack), //  start
                now                                           // end
            );
            break;
        case CODE_PROFILE_15_MINUTE:
            gap::generate_report_15_minutes(
                db,
                profile,
                root,
                cyng::sys::get_start_of_day(now - backtrack), //  start
                cyng::sys::get_end_of_day(now)                //  now
            );
            break;
        case CODE_PROFILE_60_MINUTE:
            gap::generate_report_60_minutes(
                db,
                profile,
                root,
                cyng::sys::get_start_of_month(now - backtrack), //  start
                cyng::sys::get_end_of_day(now)                  //  now
            );
            break;
        case CODE_PROFILE_24_HOUR:
            gap::generate_report_24_hour(
                db,
                profile,
                root,
                cyng::sys::get_start_of_month(now - backtrack), //  start
                cyng::sys::get_end_of_month(now)                //  now
            );
            break;
        case CODE_PROFILE_1_MONTH:
            gap::generate_report_1_month(
                db,
                profile,
                root,
                cyng::sys::get_start_of_year(now - backtrack), //  start
                cyng::sys::get_end_of_year(now)                //  now
            );
            break;
        case CODE_PROFILE_1_YEAR:
            gap::generate_report_1_year(
                db,
                profile,
                root,
                cyng::sys::get_start_of_year(now - backtrack), //  start
                cyng::sys::get_end_of_year(now)                //  now
            );
            break;

        default: break;
        }
    }

    namespace gap {
        /**
         * 1 minute reports
         */
        void generate_report_1_minute(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point end) {}

        /**
         * Quarter hour reports
         */
        void generate_report_15_minutes(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point end) {
#ifdef _DEBUG
            std::cout << "15 min (" << profile << ") reporting period spans "
                      << std::chrono::duration_cast<std::chrono::hours>(end - start) << " from "
                      << cyng::sys::to_string(start, "%F %T%z") << " to " << cyng::sys::to_string(end, "%F %T%z")
                      << ", offset = " << start << " with "
                      << sml::calculate_entry_count(profile, std::chrono::duration_cast<std::chrono::hours>(end - start))
                      << " expected entries/meter in total" << std::endl;
#endif
            //
            //  The specified time of (i.e  2022-10-07 00:00:00+0200 - during Daylight Saving Time) has to be adjusted back by 2
            //  hours to get the UTC time.
            //

            //
            //  generate gap reports about the complete time range
            //
            gap::readout_t data;
            auto const range = std::chrono::hours(24);
            for (auto idx = start; idx < end; idx += range) {

                data = generate_report_15_minutes(
                    db,
                    data, // initial data
                    profile,
                    root,
                    make_tz_offset(idx), // start (UTC)
                    range);
            }
        }

        gap::readout_t generate_report_15_minutes(
            cyng::db::session db,
            gap::readout_t const &initial_data,
            cyng::obis profile,
            std::filesystem::path root,
            tz_type start,
            std::chrono::hours span) {

            auto const end = start.utc_time() + span;
            auto const count = sml::calculate_entry_count(profile, span);

            auto const data = collect_report(db, initial_data, profile, root, start, end, start.deviation(), count);
#ifdef _DEBUG
            std::cout << "start 15 min gap report ";
            cyng::sys::to_string_utc(std::cout, start, "%Y-%m-%dT%H:%M (UTC)");
            std::cout << " => ";
            cyng::sys::to_string_utc(std::cout, end, "%Y-%m-%dT%H:%M (UTC) - ");
            std::cout << count << " expected entries for " << data.size() << " meter(s) each" << std::endl;
#endif
            return data;
        }

        /**
         * Hourly reports
         */
        void generate_report_60_minutes(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point end) {
#ifdef _DEBUG
            std::cout << "60 min (" << profile << ") gap report period spans from " << start << " to " << end << " ("
                      << std::chrono::duration_cast<std::chrono::hours>(end - start) << "), offset = ?"
                      << " minutes with "
                      << sml::calculate_entry_count(profile, std::chrono::duration_cast<std::chrono::hours>(end - start))
                      << " entries in total" << std::endl;

            std::cout << "\tmonth starts at " << cyng::sys::get_start_of_month(start) << " with a length of "
                      << cyng::sys::get_length_of_month(start) << std::endl;
#endif

            // tz_offset adj_start(start, utc_offset);

            //
            //  generate gap reports about the complete time range
            //
            gap::readout_t data;
            for (auto idx = start; idx < end;) {
                auto const range = cyng::sys::get_length_of_month(start);
                data = generate_report_60_minutes(
                    db,
                    data,
                    profile,
                    root,
                    make_tz_offset(idx), // start (UTC)
                    range                // time range 1 month
                );

                start += range;
            }
        }

        gap::readout_t generate_report_60_minutes(
            cyng::db::session db,
            gap::readout_t const &initial_data,
            cyng::obis profile,
            std::filesystem::path root,
            tz_type start,
            std::chrono::hours span) {

            auto const end = start.utc_time() + span;
            auto const count = sml::calculate_entry_count(profile, span);

            auto const data = collect_report(db, initial_data, profile, root, start, end, start.deviation(), count);
#ifdef _DEBUG
            std::cout << "start 60 min report " << start;
            std::cout << " => ";
            cyng::sys::to_string_utc(std::cout, end, "%Y-%m-%dT%H:%M (UTC) - ");
            std::cout << count << " expected entries, " << data.size() << " meter(s) each" << std::endl;
#endif
            return data;
        }

        /**
         * Daily reports
         */
        void generate_report_24_hour(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now) {
#ifdef _DEBUG
            std::cout << "24 h (" << profile << ") gap report period spans from " << start << " to " << now << " ("
                      << std::chrono::duration_cast<std::chrono::hours>(now - start) << "), offset = ? "
                      << " minutes with "
                      << sml::calculate_entry_count(profile, std::chrono::duration_cast<std::chrono::hours>(now - start))
                      << " entries in total" << std::endl;
#endif
            // tz_offset adj_start(start, utc_offset);

            //
            //  generate gap reports about the complete time range
            //
            gap::readout_t data;
            while (start < now) {

                auto const range = cyng::sys::get_length_of_month(start);
                data = generate_report_24_hour(
                    db,
                    data, // initial data
                    profile,
                    root,
                    make_tz_offset(start), // start
                    range                  //  time range 1 month
                );

                start += range;
            }
        }

        gap::readout_t generate_report_24_hour(
            cyng::db::session db,
            gap::readout_t const &initial_data,
            cyng::obis profile,
            std::filesystem::path root,
            tz_type start,
            std::chrono::hours span) {

            auto const end = start.utc_time() + span;
            auto const count = sml::calculate_entry_count(profile, span);

            auto const data = collect_report(db, initial_data, profile, root, start, end, start.deviation(), count);
#ifdef _DEBUG
            std::cout << "start 24 h report " << start;
            std::cout << " => ";
            cyng::sys::to_string(std::cout, end, "%Y-%m-%dT%H:%M%z (UTC) - ");
            std::cout << count << " expected entries, " << data.size() << " meter(s)" << std::endl;
#endif
            return data;
        }

        /**
         * Monthly reports
         */
        void generate_report_1_month(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point) {}

        /**
         * Yearly reports
         */
        void generate_report_1_year(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,

            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point) {}

        gap::readout_t collect_report(
            cyng::db::session db,
            gap::readout_t const &initial_data,
            cyng::obis profile,
            std::filesystem::path root,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point end,
            std::chrono::minutes utc_offset,
            std::size_t count) //  expected number of entries in time span
        {

            auto const first = start; // +utc_offset;
            auto const last = end;    // +utc_offset;

            auto const slot = sml::to_index(first, profile); //  start slot
            BOOST_ASSERT(slot.second);

            //
            //  collect data from this time range ordered by meter -> register -> timepoint
            //
            auto const data = collect_readouts_by_time_range(db, initial_data, profile, first, last);

            auto const file_name = get_filename("", profile, start, end);
            auto const file_path = root / file_name;
#ifdef _DEBUG
            // std::cout << profile << ": " << file_path << std::endl;
#endif
            std::ofstream of(file_path.string(), std::ios::trunc);

            if (of.is_open()) {

                //
                //  loop over all meters
                //
                if (data.empty()) {
                    of << count << std::endl;
                } else {
                    for (auto const &val : data) {
                        //
                        //  server id
                        //
                        auto const srv_id = to_srv_id(val.first);

#ifdef _DEBUG
                        std::cout << srv_id_to_str(val.first) << " has " << val.second.size() << "/" << count << " entries => "
                                  << ((val.second.size() * 100.0) / count) << "%" << std::endl;
#endif
                        //
                        // print report
                        //
                        emit_data(of, profile, srv_id, slot.first, val.second, utc_offset, count);
                    }
                }
            }
            return data;
        }

        void emit_data(
            std::ostream &os,
            cyng::obis profile,
            srv_id_t srv_id,
            std::int64_t start_slot,
            gap::slot_date_t const &data,
            std::chrono::minutes utc_offset,
            std::size_t count) //  expected number of entries in time span
        {
            os << to_string(srv_id) << ',' << utc_offset.count() << ',' << '#' << start_slot << ',' << data.size() << ',' << count
               << ',' << ((data.size() * 100.0) / count) << '%';
            for (auto slot = start_slot; slot < start_slot + count; ++slot) {

                os << ',';
                auto const pos = data.find(slot);
                if (pos != data.end()) {
                    os << "[set@";
                    cyng::sys::to_string_utc(os, pos->second, "%FT%T%z");
                    // cyng::sys::to_string_utc(os, pos->second, "%FT%T%z");
                    os << '#' << slot << ']';
                } else {
                    //  missing time point
                    os << "[pull@";
                    auto const tp = sml::to_time_point(slot, profile);
                    cyng::sys::to_string_utc(os, tp, "%FT%T%z");
                    os << '#' << slot << ']';
                }
            }
            os << std::endl;
        }
    } // namespace gap

} // namespace smf
