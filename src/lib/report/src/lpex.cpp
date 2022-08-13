
#include <smf/report/lpex.h>
#include <smf/report/utility.h>

#include <smf/config/schemes.h>
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
        std::string prefix) {

        switch (profile.to_uint64()) {
        case CODE_PROFILE_1_MINUTE:
            lpex::generate_report_1_minute(db, profile, root, prefix, cyng::sys::get_start_of_day(now - backtrack), now);
            break;
        case CODE_PROFILE_15_MINUTE:
            lpex::generate_report_15_minutes(
                db, profile, root, prefix, cyng::sys::get_start_of_day(now - backtrack), cyng::sys::get_end_of_day(now));
            break;
        case CODE_PROFILE_60_MINUTE:
            lpex::generate_report_60_minutes(
                db, profile, root, prefix, cyng::sys::get_start_of_day(now - backtrack), cyng::sys::get_end_of_day(now));
            break;
        case CODE_PROFILE_24_HOUR:
            lpex::generate_report_24_hour(
                db, profile, root, prefix, cyng::sys::get_start_of_month(now - backtrack), cyng::sys::get_end_of_month(now));
            break;
        case CODE_PROFILE_1_MONTH:
            lpex::generate_report_1_month(
                db, profile, root, prefix, cyng::sys::get_start_of_year(now - backtrack), cyng::sys::get_end_of_year(now));
            break;
        case CODE_PROFILE_1_YEAR:
            lpex::generate_report_1_year(
                db, profile, root, prefix, cyng::sys::get_start_of_year(now - backtrack), cyng::sys::get_end_of_year(now));
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
            std::chrono::system_clock::time_point now) {}

        /**
         * Quarter hour reports
         */
        void generate_report_15_minutes(
            cyng::db::session db,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point now) {
#ifdef _DEBUG
            std::cout << "15 min (" << profile << ") lpex report period spans from " << start << " to " << now << " ("
                      << std::chrono::duration_cast<std::chrono::hours>(now - start) << ")" << std::endl;
#endif
            //
            //  generate lpex reports about the complete time range
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

        /**
         * Hourly reports
         */
        void generate_report_60_minutes(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point) {}

        /**
         * Daily reports
         */
        void generate_report_24_hour(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point) {}

        /**
         * Monthly reports
         */
        void generate_report_1_month(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point) {}

        /**
         * Yearly reports
         */
        void generate_report_1_year(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point) {}

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
            srv_id_t srv_id) {

            //
            //  server id as "octet"
            //
            auto const id = to_buffer(srv_id);

            //
            // collect data from this time range
            // LPEx needs another ordering: take one register and collect all readouts in the time span
            //
            //  collect data from this time range ordered by register
            //
            std::map<cyng::obis, std::map<std::int64_t, sml_data>> const data =
                collect_data_by_register(db, profile, id, start, end);

            //
            // collect all used profiles
            //
            // auto const regs = collect_profiles(data);

#ifdef _DEBUG
            // std::cout << to_string(srv_id) << " has " << regs.size() << " registers: ";
            // for (auto const &reg : regs) {
            //     std::cout << reg << " ";
            // }
            // std::cout << std::endl;
#endif

            //
            // ToDo: print report
            //
            auto const file_name = get_filename(prefix, profile, srv_id, start);
            emit_report(root, file_name, profile, srv_id, data);
        }

        void emit_report(
            std::filesystem::path root,
            std::string file_name,
            cyng::obis profile,
            srv_id_t srv_id,
            std::map<cyng::obis, std::map<std::int64_t, sml_data>> const &data) {
            auto const file_path = root / file_name;
            std::ofstream of(file_path.string(), std::ios::trunc);

            if (of.is_open()) {
                //
                // header
                // ToDo: read "print-version"
                //
                auto const v = fill_up(get_version());
                emit_line(of, v);

                auto const h = fill_up(get_header());
                emit_line(of, h);

                //
                //  data
                //
                emit_data(of, profile, srv_id, data);
            }
        }

        void emit_data(
            std::ostream &os,
            cyng::obis profile,
            srv_id_t srv_id,
            std::map<cyng::obis, std::map<std::int64_t, sml_data>> const &data) {

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
                            auto time_slot = sml::to_time_point(pos_ro->first, profile);
                            //
                            //  time stamp (Datum)
                            //
                            // os << std::put_time(&tm, "%d-%m-%Y %H-%M-%S")
                            os << time_slot << ";";

                            //
                            //  start time (Zeit)
                            //
                            os << "00:15:00"
                               << ";";

                            //
                            //  customer data
                            //
                            emit_customer_data(os, srv_id);

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
                            //  values
                            //
                            do {
                                os << ";" << pos_ro->second.reading_ << ";0";
                                ++pos_ro;
                            } while (pos_ro != pos->second.end());
                            os << std::endl;
                        }
                    }
                    //
                    //  next register
                    //
                    ++pos;
                } while (pos != data.end());
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

        void emit_customer_data(std::ostream &os, srv_id_t srv_id) {
            //
            //  ToDo: get customer and meter data from table "meterLPEx" -> "customer"
            //

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

    } // namespace lpex

} // namespace smf
