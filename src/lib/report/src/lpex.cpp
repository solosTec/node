
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
        std::chrono::system_clock::time_point now, // local time
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
            auto const start = cyng::date::make_date_from_utc_time(now - backtrack).get_start_of_day().to_utc_time_point();

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

            report_range const rr(
                profile,
                cyng::date::make_date_from_local_time(now - backtrack).get_start_of_day(),
                cyng::date::make_date_from_local_time(now).get_end_of_day());

#ifdef _DEBUG
            //  PROFILE_15_MINUTE:2022-11-26 00:00:00 ==120h=> 2022-11-31 00:00:00
            std::cout << "rr   : " << rr << std::endl;
#endif

            lpex::generate_report_15_minutes(db, rr, filter, root, prefix, print_version, separated, debug_mode, cb);
        } break;
        case CODE_PROFILE_60_MINUTE: {

            report_range const rr(
                profile,
                cyng::date::make_date_from_utc_time(now - backtrack).get_start_of_month(),
                cyng::date::make_date_from_utc_time(now).get_end_of_day());

#ifdef _DEBUG
            std::cout << rr << std::endl;
#endif

            lpex::generate_report_60_minutes(db, rr, filter, root, prefix, print_version, separated, debug_mode, cb);
        } break;
        case CODE_PROFILE_24_HOUR: {

            report_range const rr(
                profile,
                cyng::date::make_date_from_utc_time(now - backtrack).get_start_of_month(),
                cyng::date::make_date_from_utc_time(now).get_end_of_month());

#ifdef _DEBUG
            std::cout << rr << std::endl;
#endif

            lpex::generate_report_24_hour(db, rr, filter, root, prefix, print_version, separated, debug_mode, cb);
        } break;
        case CODE_PROFILE_1_MONTH: {

            report_range const rr(
                profile,
                cyng::date::make_date_from_utc_time(now - backtrack).get_start_of_year(),
                cyng::date::make_date_from_utc_time(now).get_end_of_month());

#ifdef _DEBUG
            std::cout << rr << std::endl;
#endif

            lpex::generate_report_1_month(db, rr, filter, root, prefix, print_version, separated, debug_mode, cb);
        } break;
        case CODE_PROFILE_1_YEAR: {

            report_range const rr(
                profile,
                cyng::date::make_date_from_utc_time(now - backtrack).get_start_of_year(),
                cyng::date::make_date_from_utc_time(now).get_end_of_year());

#ifdef _DEBUG
            std::cout << rr << std::endl;
#endif

            lpex::generate_report_1_year(db, rr, filter, root, prefix, print_version, separated, debug_mode, cb);
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
            report_range const &rr,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {

            //
            //  generate lpex reports about the complete time range
            //
            auto const step = std::chrono::hours(24);
            for (auto idx = rr.get_start(); idx < rr.get_end(); idx += step) {

                report_range const subrr(rr.get_profile(), idx, step);
                generate_report_15_minutes_part(db, subrr, filter, root, prefix, print_version, separated, debug_mode, cb);
            }
        }

        void generate_report_15_minutes_part(
            cyng::db::session db,
            report_range const &subrr,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {

#ifdef _DEBUG
            //  PROFILE_15_MINUTE:2022-11-26 00:00:00 ==120h=> 2022-11-31 00:00:00
            std::cout << "subrr: " << subrr << std::endl;
#endif

            auto const size = collect_report(db, subrr, filter, root, prefix, print_version, separated, debug_mode);

            //
            //  logging callback
            //
            cb(subrr.get_start().to_local_time_point(), subrr.get_span(), size, subrr.max_readout_count());
        }

        /**
         * Hourly reports
         */
        void generate_report_60_minutes(
            cyng::db::session db,
            report_range const &rr,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
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
            auto [start, end] = rr.get_range();
            auto const step = std::chrono::hours(24);
            while (start < end) {

                report_range const subrr(rr.get_profile(), start, step);
                generate_report_60_minutes_part(db, subrr, filter, root, prefix, print_version, debug_mode, separated, cb);
                start += step;
            }
        }

        void generate_report_60_minutes_part(
            cyng::db::session db,
            report_range const &subrr,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {

            auto const size = collect_report(db, subrr, filter, root, prefix, print_version, separated, debug_mode);

            //
            //  logging callback
            //
            cb(subrr.get_start().to_utc_time_point(), subrr.get_span(), size, subrr.max_readout_count());

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
            report_range const &rr,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {

            //
            //  generate lpex reports about the complete time range
            //
            auto [start, end] = rr.get_range();
            while (start < end) {

                auto const step = start.hours_in_month();
                report_range const subrr(rr.get_profile(), start, step);

                generate_report_24_hour(db, subrr, filter, root, prefix, print_version, separated, debug_mode, cb);

                start += step;
            }
        }

        void generate_report_24_hour_part(
            cyng::db::session db,
            report_range const &subrr,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {

            auto const size = collect_report(db, subrr, filter, root, prefix, print_version, separated, debug_mode);

            //
            //  logging callback
            //
            auto const [start, end] = subrr.get_range();
            cb(start.to_utc_time_point(), subrr.get_span(), size, subrr.max_readout_count());

            // auto const end = start.utc_time() + span;
            // auto const count = sml::calculate_entry_count(profile, span);
            // auto const size =
            //     collect_report(db, profile, filter, root, prefix, start, end, start, count, print_version, separated,
            //     debug_mode);
            // cb(start.utc_time(), span, size, count);

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
            report_range const &rr,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            bool print_version,
            bool separated,
            bool debug_mode,
            lpex_cb cb) {}

        /**
         * Yearly reports
         */
        void generate_report_1_year(
            cyng::db::session,
            report_range const &rr,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
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
            report_range const &subrr,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            bool print_version,
            bool separated,
            bool debug_mode) {

            //
            //  collect data from this time range ordered by meter -> register -> timepoint
            //
            auto const data = collect_data_by_time_range(db, filter, subrr);

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
                    auto const file_name = get_filename(prefix, subrr.get_profile(), srv_id, subrr.get_start());

                    //
                    //  open stream
                    //
                    std::ofstream of = open_report(root, file_name, print_version);

                    //
                    //  write report
                    //
                    emit_report(of, subrr, srv_id, debug_mode, val.second, customer_data);
                }
            } else {
                //
                // Generate a report for *all* devices in one file.
                // loop over all meters
                //
                auto const file_name = get_filename(prefix, subrr.get_profile(), subrr.get_start());

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
                    emit_report(ofs, subrr, srv_id, debug_mode, val.second, customer_data);
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
            report_range const &subrr,
            srv_id_t srv_id,
            bool debug_mode,
            data::values_t const &data,
            // std::chrono::minutes offset, // UTC offset
            // std::size_t count,           //  entries in time span
            std::optional<lpex_customer> const &customer_data) {

            if (ofs.is_open()) {

                //
                //  data
                //
                emit_data(ofs, subrr, srv_id, debug_mode, data, customer_data);
            }
        }

        void emit_data(
            std::ostream &os,
            report_range const &subrr,
            srv_id_t srv_id,
            bool debug_mode,
            data::values_t const &data,
            std::optional<lpex_customer> const &customer_data) {

            //
            //  iterate over all registers
            //
            for (auto pos = data.begin(); pos != data.end(); ++pos) {

                //
                //  the register in this data set
                //
                auto reg = pos->first;

                auto pos_ro = pos->second.begin();
                if (pos_ro != pos->second.end()) {

                    if (pos_ro->second.unit_ != mbus::unit::UNDEFINED_) {

                        //
                        //   the first time stamp in this data set (UTC)
                        //
                        auto const time_slot = sml::restore_time_point(pos_ro->first, subrr.get_profile());

                        //
                        //  local time stamp (Datum, Zeit)
                        //
                        auto const first = cyng::date::make_date_from_utc_time(time_slot);
                        cyng::as_string(os, first, "%d.%m.%y;%H:%M:%S;");
#ifdef _DEBUG
                        auto pos_ro_last = (--pos->second.end());
                        auto const time_slot_last = sml::restore_time_point(pos_ro_last->first, subrr.get_profile());
                        auto const last = cyng::date::make_date_from_utc_time(time_slot_last);

                        std::cout << "start at " << cyng::as_string(first, "%d.%m.%y;%H:%M:%S;")
                                  << ", last: " << cyng::as_string(last, "%d.%m.%y;%H:%M:%S;") << std::endl;
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
                        os << sml::interval_time(time_slot, subrr.get_profile()).count();

                        // std::pair<std::int64_t, bool> to_index(std::chrono::system_clock::time_point now, cyng::obis profile)
                        // auto const last = sml::to_index(subrr.get_end().to_utc_time_point(), subrr.get_profile());
                        //
                        // values
                        // Emit only values that are after "first"
                        //
                        emit_values(
                            os,
                            subrr,
                            srv_id,
                            debug_mode,
                            pos_ro->first, // first time slot
                            pos->second    // readout data
                        );
                    }
                }
            }
        }

        void emit_values(
            std::ostream &os,
            report_range const &subrr,
            srv_id_t srv_id,
            bool debug_mode,
            std::uint64_t first_slot,
            std::map<std::int64_t, sml_data> const &load) {

            BOOST_ASSERT(!load.empty());

            auto const [slot_start, slot_end] = subrr.get_slots();
            BOOST_ASSERT(slot_start < slot_end);
            BOOST_ASSERT(slot_start <= first_slot);

            //
            // N entries per day
            //

            for (auto idx = first_slot; idx < slot_end; ++idx) {
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
                     cyng::to_string(subrr.get_profile()),
                     cyng::as_string(subrr.get_start(), "%d-%m-%yT%H:%M:%S"),
                     cyng::as_string(subrr.get_end(), "%d-%m-%yT%H:%M:%S"),
                     std::to_string(first_slot),
                     std::to_string(slot_end),
                     std::to_string(slot_end - first_slot),
                     std::to_string(slot_start),
                     std::to_string(first),
                     std::to_string(last),
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
