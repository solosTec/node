
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
#include <cyng/obj/intrinsics/date.h>

#include <fstream>
#include <iostream>

namespace smf {

    void generate_lpex(
        cyng::db::session db,
        cyng::obis profile,
        cyng::obis_path_t filter,
        std::filesystem::path root,
        std::chrono::hours backtrack,
        std::chrono::system_clock::time_point now,
        std::string prefix,
        bool print_version,
        bool separated,
        bool debug_mode,
        lpex_cb cb) {

        //
        //	start transaction
        //
        cyng::db::transaction trx(db);

        switch (profile.to_uint64()) {
        case CODE_PROFILE_1_MINUTE: {
            auto const start = cyng::make_date_from_local_time(now - backtrack).get_start_of_day().to_time_point();

            lpex::generate_report_1_minute(
                db,
                profile,
                filter,
                root,
                prefix,
                start, //  start
                now,
                print_version,
                separated,
                debug_mode,
                cb);
        } break;
        case CODE_PROFILE_15_MINUTE: {

            auto const start = cyng::make_date_from_local_time(now - backtrack).get_start_of_day().to_time_point();
            auto const end = cyng::make_date_from_local_time(now).get_end_of_day().to_time_point();
            lpex::generate_report_15_minutes(
                db,
                profile,
                filter,
                root,
                prefix,
                start, //  start
                end,   //  now
                print_version,
                separated,
                debug_mode,
                cb);
        } break;
        case CODE_PROFILE_60_MINUTE: {

            auto const start = cyng::make_date_from_local_time(now - backtrack).get_start_of_month().to_time_point();
            auto const end = cyng::make_date_from_local_time(now).get_end_of_day().to_time_point();

            lpex::generate_report_60_minutes(
                db,
                profile,
                filter,
                root,
                prefix,
                start, //  start
                end,   //  now
                print_version,
                separated,
                debug_mode,
                cb);
        } break;
        case CODE_PROFILE_24_HOUR: {
            auto const start = cyng::make_date_from_local_time(now - backtrack).get_start_of_month().to_time_point();
            auto const end = cyng::make_date_from_local_time(now).get_end_of_month().to_time_point();

            lpex::generate_report_24_hour(
                db,
                profile,
                filter,
                root,
                prefix,
                start, //  start
                end,   //  now
                print_version,
                separated,
                debug_mode,
                cb);
        } break;
        case CODE_PROFILE_1_MONTH: {
            auto const start = cyng::make_date_from_local_time(now - backtrack).get_start_of_year().to_time_point();
            auto const end = cyng::make_date_from_local_time(now).get_end_of_month().to_time_point();
            lpex::generate_report_1_month(
                db,
                profile,
                filter,
                root,
                prefix,
                start, //  start
                end,   //  now
                print_version,
                separated,
                debug_mode,
                cb);
        } break;
        case CODE_PROFILE_1_YEAR: {
            auto const start = cyng::make_date_from_local_time(now - backtrack).get_start_of_year().to_time_point();
            auto const end = cyng::make_date_from_local_time(now).get_end_of_year().to_time_point();
            lpex::generate_report_1_year(
                db,
                profile,
                filter,
                root,
                prefix,
                start, //  start
                end,   //  now
                print_version,
                separated,
                debug_mode,
                cb);
        } break;

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
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {
            ;
        }

        /**
         * Quarter hour reports
         */
        void generate_report_15_minutes(
            cyng::db::session db,
            cyng::obis profile,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start, // UTC
            std::chrono::system_clock::time_point end,   // UTC
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {
#ifdef _DEBUG
            // std::cout << "15 min (" << profile << ") lpex report period spans from "
            //           << cyng::sys::to_string(start, "%Y-%m-%dT%H:%M%z") << " to " << cyng::sys::to_string(end,
            //           "%Y-%m-%dT%H:%M%z")
            //           << " (" << std::chrono::duration_cast<std::chrono::hours>(end - start) << "), with "
            //           << sml::calculate_entry_count(profile, std::chrono::duration_cast<std::chrono::hours>(end - start))
            //           << " entries in total" << std::endl;
#endif
            //
            //  generate lpex reports about the complete time range
            //
            auto const range = std::chrono::hours(24);
            for (auto idx = start; idx < end; idx += range) {

                generate_report_15_minutes(
                    db,
                    profile,
                    filter,
                    root,
                    prefix,
                    make_tz_offset(idx), // compensate time difference to UTC
                    range,
                    print_version,
                    separated,
                    debug_mode,
                    cb);
            }
        }

        void generate_report_15_minutes(
            cyng::db::session db,
            cyng::obis profile,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            tz_type &&start,
            std::chrono::hours span,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {

            auto const end = start.utc_time() + span;
            auto const count = sml::calculate_entry_count(profile, span);
#ifdef _DEBUG
            // cyng::sys::to_string(std::cout, start.local_time(), "%Y-%m-%dT%H:%M%z");
            // std::cout << " (localtime), ";
            // cyng::sys::to_string(std::cout, start.utc_time(), "%Y-%m-%dT%H:%M%z");
            // std::cout << " (UTC) => ";
            // cyng::sys::to_string(std::cout, end, "%Y-%m-%dT%H:%M%z (UTC)");
            // std::cout << std::endl;
#endif
            auto const size = collect_report(
                db,
                profile,
                filter,
                root,
                prefix,
                start, // UTC adjusted
                end,   // UTC adjusted
                start, // will be converted into time difference between local and UTC
                count,
                print_version,
                separated,
                debug_mode);
            cb(start.utc_time(), span, size, count);

#ifdef __DEBUG
            std::cout << "start 15 min report ";
            cyng::sys::to_string(std::cout, start, "%Y-%m-%dT%H:%M%z");
            std::cout << " => ";
            cyng::sys::to_string(std::cout, end, "%Y-%m-%dT%H:%M%z)");
            std::cout << ", offset to UTC is " << start.deviation().count() << " minutes - entries: " << size << "/" << count
                      << std::endl;
#endif
        }

        /**
         * Hourly reports
         */
        void generate_report_60_minutes(
            cyng::db::session db,
            cyng::obis profile,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point end,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {
#ifdef _DEBUG
            // std::cout << "60 min (" << profile << ") lpex report period spans from " << start << " to " << end << " ("
            //           << std::chrono::duration_cast<std::chrono::hours>(end - start) << ") "
            //           << sml::calculate_entry_count(profile, std::chrono::duration_cast<std::chrono::hours>(end - start))
            //           << " entries in total" << std::endl;

            // std::cout << "start of month : " << cyng::sys::get_start_of_month(start) << std::endl;
            // std::cout << "length of month: " << cyng::sys::get_length_of_month(start) << std::endl;
#endif
            //
            //  generate lpex reports about the complete time range
            //
            while (start < end) {

                auto const range = cyng::make_date_from_local_time(start).hours_in_month();
                generate_report_60_minutes(
                    db,
                    profile,
                    filter,
                    root,
                    prefix,
                    make_tz_offset(start), // compensate time difference to UTC
                    range,                 // time range 1 month
                    print_version,
                    debug_mode,
                    separated,
                    cb);

                start += range;
            }
        }

        void generate_report_60_minutes(
            cyng::db::session db,
            cyng::obis profile,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            tz_type &&start,
            std::chrono::hours span,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {

            auto const end = start.utc_time() + span;
            auto const count = sml::calculate_entry_count(profile, span);
            auto const size =
                collect_report(db, profile, filter, root, prefix, start, end, start, count, print_version, separated, debug_mode);
            cb(start.utc_time(), span, size, count);

#ifdef __DEBUG
            std::cout << "start 60 min report ";
            cyng::sys::to_string_utc(std::cout, start, "%Y-%m-%dT%H:%M (UTC)");
            std::cout << " => ";
            cyng::sys::to_string_utc(std::cout, end, "%Y-%m-%dT%H:%M (UTC)");
            std::cout << ", offset to UTC is " << start.deviation().count() << " minutes - entries: " << size << "/" << count
                      << std::endl;
#endif
        }

        /**
         * Daily reports
         */
        void generate_report_24_hour(
            cyng::db::session db,
            cyng::obis profile,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {
#ifdef _DEBUG
            // std::cout << "24 h (" << profile << ") lpex report period spans from " << start << " to " << now << " ("
            //           << std::chrono::duration_cast<std::chrono::hours>(now - start) << ") "
            //           << sml::calculate_entry_count(profile, std::chrono::duration_cast<std::chrono::hours>(now - start))
            //           << " entries in total" << std::endl;
#endif
            //
            //  generate lpex reports about the complete time range
            //
            while (start < now) {

                auto const range = cyng::make_date_from_local_time(start).hours_in_month();
                // auto const range = cyng::sys::get_length_of_month(start); //  time range 1 month
                generate_report_24_hour(
                    db,
                    profile,
                    filter,
                    root,
                    prefix,
                    make_tz_offset(start), // compensate time difference to UTC
                    range,                 //  time range 1 month
                    print_version,
                    separated,
                    debug_mode,
                    cb);

                start += range;
            }
        }

        void generate_report_24_hour(
            cyng::db::session db,
            cyng::obis profile,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            tz_type &&start,
            std::chrono::hours span,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {

            auto const end = start.utc_time() + span;
            auto const count = sml::calculate_entry_count(profile, span);
            auto const size =
                collect_report(db, profile, filter, root, prefix, start, end, start, count, print_version, separated, debug_mode);
            cb(start.utc_time(), span, size, count);

#ifdef __DEBUG
            std::cout << "start 24 h report ";
            cyng::sys::to_string_utc(std::cout, start, "%Y-%m-%dT%H:%M (UTC)");
            std::cout << " => ";
            cyng::sys::to_string_utc(std::cout, end, "%Y-%m-%dT%H:%M (UTC)");
            std::cout << ", offset to UTC is " << start.deviation().count() << " minutes - entries: " << size << "/" << count
                      << std::endl;
#endif
        }

        /**
         * Monthly reports
         */
        void generate_report_1_month(
            cyng::db::session,
            cyng::obis profile,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {}

        /**
         * Yearly reports
         */
        void generate_report_1_year(
            cyng::db::session,
            cyng::obis profile,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {}

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

        std::size_t collect_report(
            cyng::db::session db,
            cyng::obis profile,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start, // UTC adjusted time
            std::chrono::system_clock::time_point end,   // UTC adjusted time
            std::chrono::minutes offset,
            std::size_t count, //  expected number of entries in time span
            bool print_version,
            bool separated,
            bool debug_mode) {

            //
            //  collect data from this time range ordered by meter -> register -> timepoint
            //
            auto const data = collect_data_by_time_range(db, profile, filter, start, end);

            if (separated) {
                //
                // Generate reports for every device individually
                // loop over all meters
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
                    // print report (add offset to get the local time)
                    //
                    auto const file_name = get_filename(prefix, profile, srv_id, start - offset);

                    //
                    //  open stream
                    //
                    std::ofstream of = open_report(root, file_name, print_version);

                    //
                    //  write report
                    //
                    emit_report(of, profile, srv_id, debug_mode, val.second, offset, count, customer_data);
                }
            } else {
                //
                // Generate a report for *all* devices in one file.
                // loop over all meters
                //
                auto const file_name = get_filename(prefix, profile, start - offset);

                //
                //  open stream
                //
                std::ofstream ofs = open_report(root, file_name, print_version);

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
                    // print report (add offset to get the local time)
                    //
                    emit_report(ofs, profile, srv_id, debug_mode, val.second, offset, count, customer_data);
                }
            }

            return data.size();
        }

        std::ofstream open_report(std::filesystem::path root, std::string file_name, bool print_version) {
            auto const file_path = root / file_name;
            std::ofstream ofs(file_path.string(), std::ios::trunc);

            if (ofs.is_open()) {

                //
                // "print.version"
                // LPEX V2.0
                //
                if (print_version) {
                    //  only the version info, no more ';'
                    emit_line(ofs, get_version());
                }

                //
                // header
                //
                auto const h = fill_up(get_header());
                emit_line(ofs, h);
            }

            return ofs; // move
        }

        void emit_report(
            std::ofstream &ofs,
            cyng::obis profile,
            srv_id_t srv_id,
            bool debug_mode,
            data::values_t const &data,
            std::chrono::minutes offset, // UTC offset
            std::size_t count,           //  entries in time span
            std::optional<lpex_customer> const &customer_data) {

            if (ofs.is_open()) {

                //
                //  data
                //
                emit_data(ofs, profile, srv_id, debug_mode, data, offset, count, customer_data);
            }
        }

        void emit_data(
            std::ostream &os,
            cyng::obis profile,
            srv_id_t srv_id,
            bool debug_mode,
            data::values_t const &data,
            std::chrono::minutes offset, // UTC offset
            std::size_t count,           //  entries in time span
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
                            auto const time_slot = sml::to_time_point(pos_ro->first, profile) - offset;

                            //
                            //  local time stamp (Datum, Zeit)
                            //
                            auto const d = cyng::make_date_from_local_time(time_slot);
                            cyng::as_string(os, d, "%d.%m.%y;%H:%M:%S;");
                            // cyng::sys::to_string(os, time_slot, "%d.%m.%y;%H:%M:%S;");
#ifdef _DEBUG
                            // cyng::sys::to_string_utc(std::cout, time_slot, "%Y-%m-%dT%H:%M (UTC)\n");
                            // cyng::sys::to_string(std::cout, time_slot, "%Y-%m-%dT%H:%M (local)\n");
#endif

                            //
                            //  [3..10] customer data
                            //
                            emit_customer_data(os, srv_id, customer_data);

                            //
                            //  [11] register (Kennzahl)
                            //
                            obis::to_decimal(os, reg);
                            os << ";";

                            //
                            //  [12] unit (Einheit)
                            //
                            os << mbus::get_name(pos_ro->second.unit_);
                            os << ";";

                            //  [13] Wandlerfaktor - conversion factor
                            os << "1;";

                            //  [14] Messperiodendauer (15/60) - measuring period in minutes
                            os << sml::interval_time(time_slot, profile).count();

                            //
                            // values
                            //
                            emit_values(os, profile, srv_id, debug_mode, d.get_start_of_day().to_time_point(), count, pos->second);
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
            bool debug_mode,
            std::chrono::system_clock::time_point start_day, //  start of day
            std::size_t count,                               //  entries in time span
            std::map<std::int64_t, sml_data> const &load) {

            BOOST_ASSERT(!load.empty());

            // detect gaps
            auto const start_data = sml::to_time_point(load.begin()->first, profile);
            auto const idx_day = sml::to_index(start_day, profile);
            if (debug_mode) {
                //  example: 2022-08-13 00:00:00.0000000/2022-08-13 07:30:00.0000000

                // std::cout << to_string(srv_id) << ": " << cyng::sys::to_string_utc(start_day, "%Y-%m-%dT%H:%M") << "/"
                //           << cyng::sys::to_string_utc(start_data, "%Y-%m-%dT%H:%M") << " - " << idx_day.first << "/"
                //           << load.begin()->first << " - " << load.size() << " entries" << std::endl;
            }

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
                    if (debug_mode) {
                        //  debug build print index number
                        os << ";[" << idx << "];0";
                    } else {
                        os << ";;0";
                    }

                } else {
                    //
                    //  data exists
                    //
                    os << ";" << pos->second.reading_ << ";";
                    //  status
                    if (pos->second.status_ != 0) {
                        os << std::hex << std::setfill('0') << std::setw(5) << pos->second.status_ << std::dec;
                    } else {
                        os << pos->second.status_;
                    }
                }
            }
            os << std::endl;

            if (debug_mode) {

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
            }
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
                os << ";" << customer_data->id_          // [3] Kundennummer, example: 11026661
                   << ";" << customer_data->name_        // [4] Kundenname; example: Frey Sarah
                   << ";" << customer_data->unique_name_ // [5] eindeutigeKDNr, example: AZ Bornfeldstrasse 2
                   << ";" << get_id(srv_id)              // [6] GEId (GeräteID), METERID
                   << ";"                                // [7] GEKANr
                   << ";"                                // [7] KALINr (Kanalnummer)
                   << ";"                                // [8] Linie (Liniennummer)
                   << ";"                                // [9] Linienbezeichnung
                   << ";" << customer_data->mc_          // [10] eindeutigeLINr (metering code)
                    ;

            } else {

                //
                //  (Kundennummer;Kundenname;eindeutigeKDNr)
                //
                os << ";" // [3] Kundennummer
                   << ";" // [4] Kundenname
                   << ";" // [5] eindeutigeKDNr
                    ;

                //
                // [6] meter id (GEId / GeräteID)
                //
                os << get_id(srv_id) << ";";

                //
                // GEKANr; KALINr; Linie; eindeutigeLINr; ZPB)
                //
                os << ";" // [7] GEKANr
                   << ";" // [8] KALINr (Kanalnummer)
                   << ";" // [9] Linie (Liniennummer)
                   << ";" // [10] Linienbezeichnung
#ifdef _DEBUG
                   << "CH" // MC followed by area id and location id
#endif
                   << ";" // [11] eindeutigeLINr (metering code)
                    ;
            }
            // [12] Kennzahl (OBIS)
            // [13] Einheit
        }

    } // namespace lpex

} // namespace smf
