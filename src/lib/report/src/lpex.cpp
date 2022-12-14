
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

#include <fstream>
#include <iostream>

#include <boost/uuid/uuid_io.hpp>

namespace smf {

    void generate_lpex(
        cyng::db::session db,
        cyng::obis profile,
        cyng::obis_path_t filter,
        std::filesystem::path root,
        std::string prefix,
        cyng::date now,
        std::chrono::hours backtrack,
        bool print_version,
        bool separated,
        bool debug_mode,
        bool customer) {

        //
        //	start transaction
        //
        cyng::db::transaction trx(db);

        //
        //  loop
        //
        auto const start = (now - backtrack).get_start_of_day();
#ifdef _DEBUG
        std::cout << "start at: " << cyng::as_string(start) << std::endl;
#endif

        //
        //  data set of specific time period
        //
        data::data_set_t data_set;

        //
        //  reference date
        //
        cyng::date prev = start;

        loop_readout_data(
            db,
            profile,
            start,
            [&](bool next_record,
                boost::uuids::uuid tag,
                srv_id_t id,
                cyng::date act_time,
                std::uint32_t status,
                cyng::obis reg,
                std::string value,
                std::uint16_t code,
                std::int8_t scaler,
                mbus::unit unit) -> bool {
                //
                //  calculate slot
                //
                auto const sr = sml::to_index(act_time, profile);
                BOOST_ASSERT(sr.second);
                auto const d = sml::from_index_to_date(sr.first, profile);

                if (next_record) {

                    if (sml::is_new_reporting_period(profile, prev, d)) {
                        //
                        //  new day - generate report and clear data set
                        //
                        if (!data_set.empty()) {
                            auto const file_name = get_filename(prefix, profile, prev);
                            std::cout << ">> generate report " << root / file_name << std::endl;
                            auto ofs = lpex::open_report(root, file_name, print_version);
                            if (ofs.is_open()) {
                                lpex::generate_report(db, ofs, profile, prev, data_set, customer);
                                data::clear(data_set);
                            }
                        }
                    }

                    //
                    //  update time stamp
                    //
                    prev = d;

                    std::cout << "> " << tag << ": " << to_string(id) << ", " << cyng::as_string(act_time, "%Y-%m-%d %H:%M:%S")
                              << ", " << cyng::as_string(d, "%Y-%m-%d %H:%M:%S") << std::endl;
                }

                //
                //  update data set
                //
                if (has_passed(reg, filter)) {
                    std::cout << reg << ": " << value << " " << mbus::get_name(unit) << std::endl;
                    data::update(data_set, id, reg, sr.first, code, scaler, mbus::to_u8(unit), value, status);
                } else {
                    std::cout << "> skip " << reg << ": " << value << " " << mbus::get_name(unit) << std::endl;
                }
                return true;
            });

        //
        //  generate report for last data set
        //
        if (!data_set.empty()) {
            auto const file_name = get_filename(prefix, profile, prev);
            std::cout << ">> generate report " << root / file_name << std::endl;
            auto ofs = lpex::open_report(root, file_name, print_version);
            if (ofs.is_open()) {
                lpex::generate_report(db, ofs, profile, prev, data_set, customer);
                ofs.close();
            }
            data::clear(data_set);
        }
    }

    namespace lpex {

        void generate_report(
            cyng::db::session db,
            std::ofstream &ofs,
            cyng::obis profile,
            cyng::date const &d,
            data::data_set_t const &data_set,
            bool customer) {

            auto const [start, end] = sml::to_index_range(d, profile);
            BOOST_ASSERT(start <= end);
            std::cout << "> generate report for " << data_set.size() << " meters from " << start << " = "
                      << cyng::as_string(sml::from_index_to_date(start, profile), "%Y-%m-%d %H:%M:%S") << " to " << end << " = "
                      << cyng::as_string(sml::from_index_to_date(end, profile), "%Y-%m-%d %H:%M:%S") << std::endl;

            //
            //  meter -> register -> slot -> data
            //
            for (auto const &data : data_set) {

                //
                //  customer data
                //  This slows down the overall performance. Maybe caching is a good idea
                //
                auto customer_data =
                    customer ? query_customer_data_by_meter(db, smf::to_buffer(data.first)) : std::optional<lpex_customer>();

                for (auto const &[reg, values] : data.second) {
                    if (!values.empty()) {
                        //
                        //  first time point
                        //
                        auto const first = values.begin()->first;
                        auto const unit = values.begin()->second.unit_;
                        BOOST_ASSERT(start <= first);
                        std::cout << cyng::as_string(d, "%d.%m.%y;%H:%M:%S;") << get_id(data.first) << ';' << obis::to_decimal(reg)
                                  << ';' << mbus::get_name(unit);
#ifdef _DEBUG
                        std::cout << ';' << '[' << start << ']' << ';' << '[' << first << ']' << ';' << '[' << end << ']';
#endif
                        //
                        // report
                        // start with the timestamp of the first entry
                        //
                        ofs << cyng::as_string(sml::from_index_to_date(first, profile), "%d.%m.%y;%H:%M:%S;");

                        //
                        //  [3..10] customer data
                        //
                        emit_customer_data(ofs, data.first, customer_data);

                        //
                        //  [11] register (Kennzahl)
                        //
                        obis::to_decimal(ofs, reg);
                        ofs << ";";

                        //
                        //  [12] unit (Einheit)
                        //
                        ofs << mbus::get_name(unit);
                        ofs << ";";

                        //  [13] Wandlerfaktor - conversion factor
                        ofs << "1;";

                        //  [14] Messperiodendauer (15/60) - measuring period in minutes
                        ofs << sml::interval_time(d, profile).count();

                        auto idx = start;
                        for (auto const [slot, ro] : values) {
                            //
                            //  value
                            //
                            std::cout << ';' << ro.reading_;
                            ofs << ";" << ro.reading_ << ";";

                            //
                            //  status
                            //
                            if (ro.status_ != 0) {
                                ofs << std::hex << std::setfill('0') << std::setw(5) << ro.status_ << std::dec;
                            } else {
                                ofs << ro.status_;
                            }

                            //
                            //  update slot
                            //
                            idx = slot;
                        }
                        for (; idx < end; ++idx) {
                            std::cout << ';';
                            ofs << ';';
#ifdef _DEBUG
                            std::cout << '[' << idx - start << ']';
#endif
                        }
                        std::cout << std::endl;
                        ofs << std::endl;
                    }
                }
            }
        }

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

        //

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
