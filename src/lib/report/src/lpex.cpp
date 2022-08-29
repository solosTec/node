
#include <smf/report/lpex.h>

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

    void generate_lpex(
        cyng::db::session db,
        cyng::obis profile,
        std::filesystem::path root,
        std::chrono::hours backtrack,
        std::chrono::system_clock::time_point now,
        std::string prefix,
        std::chrono::minutes utc_offset,
        bool print_version) {

        //
        //	start transaction
        //
        cyng::db::transaction trx(db);

        switch (profile.to_uint64()) {
        case CODE_PROFILE_1_MINUTE:
            lpex::generate_report_1_minute(
                db, profile, root, prefix, cyng::sys::get_start_of_day(now - backtrack), now, utc_offset, print_version);
            break;
        case CODE_PROFILE_15_MINUTE:
            lpex::generate_report_15_minutes(
                db,
                profile,
                root,
                prefix,
                cyng::sys::get_start_of_day(now - backtrack), //  start
                cyng::sys::get_end_of_day(now),               //  now
                utc_offset,
                print_version);
            break;
        case CODE_PROFILE_60_MINUTE:
            lpex::generate_report_60_minutes(
                db,
                profile,
                root,
                prefix,
                cyng::sys::get_start_of_month(now - backtrack),
                cyng::sys::get_end_of_day(now),
                utc_offset,
                print_version);
            break;
        case CODE_PROFILE_24_HOUR:
            lpex::generate_report_24_hour(
                db,
                profile,
                root,
                prefix,
                cyng::sys::get_start_of_month(now - backtrack),
                cyng::sys::get_end_of_month(now),
                utc_offset,
                print_version);
            break;
        case CODE_PROFILE_1_MONTH:
            lpex::generate_report_1_month(
                db,
                profile,
                root,
                prefix,
                cyng::sys::get_start_of_year(now - backtrack),
                cyng::sys::get_end_of_year(now),
                utc_offset,
                print_version);
            break;
        case CODE_PROFILE_1_YEAR:
            lpex::generate_report_1_year(
                db,
                profile,
                root,
                prefix,
                cyng::sys::get_start_of_year(now - backtrack),
                cyng::sys::get_end_of_year(now),
                utc_offset,
                print_version);
            break;

        default: break;
        }
    }

    namespace lpex {
        /**
         * 1 minute reports
         */
        void generate_report_1_minute(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now,
            std::chrono::minutes utc_offset,
            bool print_version) {}

        /**
         * Quarter hour reports
         */
        void generate_report_15_minutes(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now,
            std::chrono::minutes utc_offset,
            bool print_version) {
#ifdef _DEBUG
            std::cout << "15 min (" << profile << ") lpex report period spans from " << start << " to " << now << " ("
                      << std::chrono::duration_cast<std::chrono::hours>(now - start) << "), offset = " << utc_offset.count()
                      << " minutes with "
                      << sml::calculate_entry_count(profile, std::chrono::duration_cast<std::chrono::hours>(now - start))
                      << " entries in total" << std::endl;
#endif
            //
            //  generate lpex reports about the complete time range
            //
            while (start < now) {

                start = generate_report_15_minutes(
                    db,
                    profile,
                    root,
                    prefix,
                    start + utc_offset, // compensate time difference to UTC
                    std::chrono::hours(24),
                    print_version);
            }
        }

        std::chrono::system_clock::time_point generate_report_15_minutes(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::hours span,
            bool print_version) {

            auto const end = start + span;
            auto const count = sml::calculate_entry_count(profile, span);

#ifdef _DEBUG
            std::cout << "start 15 min report ";
            cyng::sys::to_string_utc(std::cout, start, "%Y-%m-%dT%H:%M (UTC)");
            std::cout << " => ";
            cyng::sys::to_string_utc(std::cout, end, "%Y-%m-%dT%H:%M (UTC) - entries: ");
            std::cout << count << std::endl;
#endif

            collect_report(db, profile, root, prefix, start, end, count, print_version);

            return end;
        }

        /**
         * Hourly reports
         */
        void generate_report_60_minutes(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now,
            std::chrono::minutes utc_offset,
            bool print_version) {
#ifdef _DEBUG
            std::cout << "60 min (" << profile << ") lpex report period spans from " << start << " to " << now << " ("
                      << std::chrono::duration_cast<std::chrono::hours>(now - start) << "), offset = " << utc_offset.count()
                      << " minutes with "
                      << sml::calculate_entry_count(profile, std::chrono::duration_cast<std::chrono::hours>(now - start))
                      << " entries in total" << std::endl;

            std::cout << "start of month : " << cyng::sys::get_start_of_month(start) << std::endl;
            std::cout << "length of month: " << cyng::sys::get_length_of_month(start) << std::endl;
#endif
            //
            //  generate lpex reports about the complete time range
            //
            while (start < now) {

                start = generate_report_60_minutes(
                    db,
                    profile,
                    root,
                    prefix,
                    start + utc_offset,                    // compensate time difference to UTC
                    cyng::sys::get_length_of_month(start), // time range 1 month
                    print_version);
            }
        }

        std::chrono::system_clock::time_point generate_report_60_minutes(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::hours span,
            bool print_version) {

            auto const end = start + span;
            auto const count = sml::calculate_entry_count(profile, span);

#ifdef _DEBUG
            std::cout << "start 60 min report ";
            cyng::sys::to_string_utc(std::cout, start, "%Y-%m-%dT%H:%M (UTC)");
            std::cout << " => ";
            cyng::sys::to_string_utc(std::cout, end, "%Y-%m-%dT%H:%M (UTC) - max. entries: ");
            std::cout << count << std::endl;
#endif

            collect_report(db, profile, root, prefix, start, end, count, print_version);

            return end;
        }

        /**
         * Daily reports
         */
        void generate_report_24_hour(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now,
            std::chrono::minutes utc_offset,
            bool print_version) {
#ifdef _DEBUG
            std::cout << "24 h (" << profile << ") lpex report period spans from " << start << " to " << now << " ("
                      << std::chrono::duration_cast<std::chrono::hours>(now - start) << "), offset = " << utc_offset.count()
                      << " minutes with "
                      << sml::calculate_entry_count(profile, std::chrono::duration_cast<std::chrono::hours>(now - start))
                      << " entries in total" << std::endl;
#endif
            //
            //  generate lpex reports about the complete time range
            //
            while (start < now) {

                start = generate_report_24_hour(
                    db,
                    profile,
                    root,
                    prefix,
                    start + utc_offset,                    // compensate time difference to UTC
                    cyng::sys::get_length_of_month(start), //  time range 1 month
                    print_version);
            }
        }

        std::chrono::system_clock::time_point generate_report_24_hour(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::hours span,
            bool print_version) {

            auto const end = start + span;
            auto const count = sml::calculate_entry_count(profile, span);

#ifdef _DEBUG
            std::cout << "start 24 h report ";
            cyng::sys::to_string_utc(std::cout, start, "%Y-%m-%dT%H:%M (UTC)");
            std::cout << " => ";
            cyng::sys::to_string_utc(std::cout, end, "%Y-%m-%dT%H:%M (UTC) - entries: ");
            std::cout << count << std::endl;
#endif

            collect_report(db, profile, root, prefix, start, end, count, print_version);

            return end;
        }

        /**
         * Monthly reports
         */
        void generate_report_1_month(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            std::chrono::minutes utc_offset,
            bool print_version) {}

        /**
         * Yearly reports
         */
        void generate_report_1_year(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            std::chrono::minutes utc_offset,
            bool print_version) {}

        std::vector<std::string> get_header() {
            return {
                "Datum",
                "Zeit",
                "Kundennummer",
                "Kundenname",
                "eindeutigeKDNr",
                "GEId",
                "GEKANr",
                "KALINr",
                "Linie",
                "eindeutigeLINr",
                "ZPB",
                "Kennzahl",
                "Einheit",
                "Wandlerfaktor",
                "MPDauer",
                "Werte" //
                // ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
            };
        }

        std::vector<std::string> fill_up(std::vector<std::string> &&vec) {
            if (vec.size() < 2 * 96) {
                vec.insert(vec.end(), (2 * 96) - vec.size(), "");
            }
            return vec;
        }

        std::vector<std::string> get_version() { return {"LPEX V2.0"}; }

        void collect_report(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point end,
            std::size_t count, //  entries in time span
            bool print_version) {

            //
            //  collect data from this time range ordered by meter -> register -> timepoint
            //
            auto const data = collect_data_by_time_range(db, profile, start, end);

            //
            //  loop over all meters
            //
            for (auto const &val : data) {
                //
                //  server id
                //
                auto const srv_id = to_srv_id(val.first);

                //
                //  customer data (if any)
                //
                auto customer_data = query_customer_data_by_meter(db, val.first);

                //
                // print report
                //
                auto const file_name = get_filename(prefix, profile, srv_id, start);
                emit_report(root, file_name, profile, srv_id, print_version, val.second, count, customer_data);
            }
        }

        void emit_report(
            std::filesystem::path root,
            std::string file_name,
            cyng::obis profile,
            srv_id_t srv_id,
            bool print_version,
            data::values_t const &data,
            std::size_t count, //  entries in time span
            std::optional<lpex_customer> const &customer_data) {

            auto const file_path = root / file_name;
#ifdef _DEBUG
            std::cout << profile << ": " << file_path << std::endl;
#endif
            std::ofstream of(file_path.string(), std::ios::trunc);

            if (of.is_open()) {
                //
                // header
                // "print-version"
                //
                if (print_version) {
                    auto const v = fill_up(get_version());
                    emit_line(of, v);
                }

                auto const h = fill_up(get_header());
                emit_line(of, h);

                //
                //  data
                //
                emit_data(of, profile, srv_id, data, count, customer_data);
            }
        }

        void emit_data(
            std::ostream &os,
            cyng::obis profile,
            srv_id_t srv_id,
            data::values_t const &data,
            std::size_t count, //  entries in time span
            std::optional<lpex_customer> const &customer_data) {

            auto pos = data.begin();
            if (pos != data.end()) {
                do {
                    //
                    //  the register in this data set
                    //
                    auto reg = pos->first;
                    auto pos_ro = pos->second.begin();
                    if (pos_ro != pos->second.end()) {

                        if (pos_ro->second.unit_ != mbus::unit::UNDEFINED_) {
                            //
                            //   the first time stamp in this data set
                            //
                            auto const time_slot = sml::to_time_point(pos_ro->first, profile);
                            //
                            //  time stamp (Datum)
                            //
                            // os << std::put_time(&tm, "%d-%m-%Y %H-%M-%S")
                            cyng::sys::to_string_utc(os, time_slot, "%y.%m.%d;");

                            //
                            //  start time (Zeit)
                            //
                            cyng::sys::to_string_utc(os, time_slot, "%H:%M:%S;");
                            // os << "00:15:00"
                            //    << ";";

                            //
                            //  customer data
                            //
                            emit_customer_data(os, srv_id, customer_data);

                            //
                            //  register (Kennzahl)
                            //
                            obis::to_decimal(os, reg);
                            os << ";";

                            //
                            //  unit (Einheit)
                            //
                            os << mbus::get_name(pos_ro->second.unit_);

                            //
                            // values
                            //
                            emit_values(os, profile, srv_id, cyng::sys::get_start_of_day(time_slot), count, pos->second);
                        }
                    }
                    //
                    //  next register
                    //
                    ++pos;
                } while (pos != data.end());
            }
        }

        void emit_values(
            std::ostream &os,
            cyng::obis profile,
            srv_id_t srv_id,
            std::chrono::system_clock::time_point start_day, //  start of day
            std::size_t count,                               //  entries in time span
            std::map<std::int64_t, sml_data> const &load) {

            BOOST_ASSERT(!load.empty());

            // detect gaps
            auto const start_data = sml::to_time_point(load.begin()->first, profile);
            auto const idx_day = sml::to_index(start_day, profile);
#ifdef _DEBUG
            //  example: 2022-08-13 00:00:00.0000000/2022-08-13 07:30:00.0000000

            std::cout << to_string(srv_id) << ": " << start_day << "/" << start_data << " - " << idx_day.first << "/"
                      << load.begin()->first << " - " << load.size() << " entries" << std::endl;
#endif

            //
            // 96 entries per day
            // ToDo: calculate this number from profile and time span
            //

            for (auto idx = idx_day.first; idx < (idx_day.first + count); ++idx) {
                auto const pos = load.find(idx);
                if (pos == load.end()) {
                    //
                    //  no data for this slot
                    //
#ifdef _DEBUG
                    //  debug build print index number
                    os << ";[" << idx << "];0";
#else
                    os << ";;0";
#endif

                } else {
                    //
                    //  data exists
                    //
                    os << ";" << pos->second.reading_ << ";" << pos->second.status_;
                }
            }
            os << std::endl;

#ifdef _DEBUG

            auto const first = load.begin()->first;
            auto const last = (--load.end())->first;
            auto const h = fill_up(
                {"[DEBUG]",
                 cyng::to_string(profile),
                 std::to_string(idx_day.first),
                 std::to_string(idx_day.first + count),
                 std::to_string(first),
                 std::to_string(last),
                 std::to_string(count),
                 std::to_string(load.size())});
            emit_line(os, h);

#endif
        }

        void emit_line(std::ostream &os, std::vector<std::string> const vec) {
            bool init = false;
            for (auto const &s : vec) {
                if (init) {
                    os << ";";
                } else {
                    init = true;
                }
                os << s;
            }
            os << std::endl;
        }

        void emit_customer_data(std::ostream &os, srv_id_t srv_id, std::optional<lpex_customer> const &customer_data) {
            //
            //  get customer and meter data from table "meterLPEx" -> "customer"
            //
            if (customer_data) {
                //  ;11026661;Frey Sarah;AZ Bornfeldstrasse 2;METERID;;;;;CH1015201234500000000000000032422;
                os << ";" << customer_data->id_          // 11026661
                   << ";" << customer_data->name_        // Frey Sarah
                   << ";" << customer_data->unique_name_ // AZ Bornfeldstrasse 2
                   << ";" << get_id(srv_id)              // METERID
                   << ";;;;"                             //
                   << ";" << customer_data->mc_ << ";";  // metering code

            } else {

                //
                //  (Kundennummer;Kundenname;eindeutigeKDNr)
                //
                os << ";;;";

                //
                // meter id (GEId)
                //
                os << get_id(srv_id) << ";";

                //
                // GEKANr; KALINr; Linie; eindeutigeLINr; ZPB)
                //
                os << ";;;;;";
            }
        }

    } // namespace lpex

} // namespace smf
