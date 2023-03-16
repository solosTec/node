
#include <smf/report/feed.h>

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

    void generate_feed(
        cyng::db::session db,
        cyng::obis profile,
        cyng::obis_path_t filter,
        std::filesystem::path root,
        std::string prefix,
        cyng::date now,
        std::chrono::hours backtrack,
        bool print_version,
        bool debug_mode,
        bool customer) {

        //
        //	start transaction
        //
        cyng::db::transaction trx(db);

        //
        // We need two time units of lead.
        //
        auto start = feed::calculate_start_time(now, backtrack, profile);
        cyng::date next_stop = start + sml::reporting_period(profile, start);

#ifdef _DEBUG
        std::cout << "start at  : " << cyng::as_string(start) << std::endl;
        std::cout << "1. stop at: " << cyng::as_string(next_stop) << std::endl;
#endif

        //
        //  data set of specific time period
        //
        data::data_set_t data_set;

        //
        //  reference date
        //
        cyng::date prev = start;

        //
        // Enter loop
        // We need at least 2 values to calculate a value advance, because this is the difference
        // of 2 values. Addionally these 2 values should follow each other directly.
        //
        loop_readout_data(
            db,
            profile,
            start - std::chrono::minutes(10),
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
                BOOST_ASSERT_MSG(sr.second, "invalid time slot");

                //
                // floor() - get timestamp from slot
                // This guaranties that the timestamp is properly aligned
                // in the time frame.
                //
                auto const d = sml::from_index_to_date(sr.first, profile);

                //
                //  is also true for the very first record
                //
                if (next_record) {

                    //
                    //  reporting advance
                    //
                    if (d > next_stop) {

                        std::cout << "next stop at: " << cyng::as_string(next_stop) << std::endl;
                        if (!data_set.empty()) {
                            auto const file_name = get_filename(prefix, profile, prev);
                            std::cout << ">> generate feed report " << root / file_name << std::endl;
                            auto ofs = feed::open_report(root, file_name, print_version);
                            if (ofs.is_open()) {
                                feed::generate_report(db, ofs, profile, start, next_stop, data_set, debug_mode, customer);
                            }
                            //
                            //  the meter list itself remains unchanged
                            //
                            // data::clear(data_set);
                        } else {
                            std::cerr << "***warning: no data" << std::endl;
                        }

                        //
                        //  update next upper limit
                        //
                        start = next_stop;
                        next_stop = next_stop + sml::reporting_period(profile, d);
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
                    // std::cout << "> skip " << reg << ": " << value << " " << mbus::get_name(unit) << std::endl;
                }
                return true;
            });

        //
        //  generate report for last data set
        //
        if (!data_set.empty()) {
            auto const file_name = get_filename(prefix, profile, prev);
            std::cout << ">> generate feed report " << root / file_name << std::endl;
            auto ofs = feed::open_report(root, file_name, print_version);
            if (ofs.is_open()) {
                feed::generate_report(db, ofs, profile, start, next_stop, data_set, debug_mode, customer);
                ofs.close();
            }
            data::clear(data_set);
        }
    }

    namespace feed {

        void generate_report(
            cyng::db::session db,
            std::ofstream &ofs,
            cyng::obis profile,
            cyng::date const &start,
            cyng::date const &end,
            data::data_set_t const &data_set,
            bool debug_mode,
            bool customer) {

            BOOST_ASSERT(start <= end);

            //
            //  start index
            //
            auto const idx_start = sml::to_index(start, profile).first;
            auto const idx_end = sml::to_index(end, profile).first;
            std::cout << ">> generate feed report for " << data_set.size() << " meters from #" << idx_start << " = "
                      << cyng::as_string(start, "%Y-%m-%d %H:%M:%S") << " to #" << idx_end << " = "
                      << cyng::as_string(end, "%Y-%m-%d %H:%M:%S") << " = " << (idx_end - idx_start) << std::endl;

            //
            //  meter -> register -> slot -> data
            //
            for (auto const &data : data_set) {

                if (is_null(data.first)) {
#ifdef _DEBUG
                    std::cerr << "skip \"null\" meter" << std::endl;
#endif
                    continue;
                }

                //
                //  customer data
                //  This slows down the overall performance. Maybe caching is a good idea
                //
                auto customer_data =
                    customer ? query_customer_data_by_meter(db, smf::to_buffer(data.first)) : std::optional<lpex_customer>();

#ifdef _DEBUG
                std::cout << ">> meter: " << to_string(data.first) << " has " << data.second.size() << " registers: ";
                for (auto const &[reg, values] : data.second) {
                    obis::to_decimal(std::cout, reg);
                    std::cout << "#" << values.size() << " ";
                }
                std::cout << std::endl;
#endif
                for (auto const &[reg, values] : data.second) {
                    //
                    //  only profiles (Lastgänge) listed
                    //
                    auto const [reg_profile, ok] = convert_obis(reg);
                    if (!values.empty() && ok) {

                        //
                        //  values at the beginning of the line
                        //
                        auto const unit = values.begin()->second.unit_;

                        //
                        // report
                        // start with the timestamp of the first entry
                        //
                        ofs << cyng::as_string(sml::from_index_to_date(idx_start + 2, profile), "%d.%m.%y;%H:%M:%S;");

                        //
                        //  [3..10] customer data
                        //
                        emit_customer_data(ofs, data.first, customer_data);

                        //
                        //  [11] register (Kennzahl)
                        //
                        obis::to_decimal(ofs, reg_profile);
                        ofs << ";";

                        //
                        //  [12] unit (Einheit)
                        //
                        ofs << mbus::get_name(unit);
                        ofs << ";";

                        //  [13] Wandlerfaktor - conversion factor
                        ofs << "1;";

                        //  [14] Messperiodendauer (15/60) - measuring period in minutes
                        ofs << sml::interval_time(end, profile).count();

                        //
                        //  loop over reporting period
                        //
                        for (auto idx = idx_start; idx < idx_end; ++idx) {

                            auto const d = sml::from_index_to_date(idx, profile);
                            // std::cout << idx << ", ";
                            auto const pos_0 = values.find(idx + 0);
                            auto const pos_1 = values.find(idx + 1);
                            if (pos_0 != values.end() && pos_1 != values.end()) {

                                //  calculate advance
                                auto const adv = calculate_advance(pos_0->second, pos_1->second);
                                if (debug_mode) {
                                    ofs << ";[" << pos_1->second.reading_ << "-" << pos_0->second.reading_ << "=" << adv << "];";
                                } else {
                                    ofs << ";" << adv << ";";
                                }

                                //
                                //   status
                                //
                                if (pos_0->second.status_ != 0) {
                                    ofs << std::hex << std::setfill('0') << std::setw(5) << pos_0->second.status_ << std::dec;
                                } else {
                                    ofs << pos_0->second.status_;
                                }

                            } else {
                                //
                                //  no value
                                //
                                ofs << ";;";
                            }
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

        cyng::date calculate_start_time(cyng::date const &now, std::chrono::hours const &backtrack, cyng::obis const &profile) {

            switch (profile.to_uint64()) {
            case CODE_PROFILE_1_MINUTE:
            case CODE_PROFILE_15_MINUTE:
            case CODE_PROFILE_60_MINUTE:
            case CODE_PROFILE_24_HOUR: return (now - backtrack).get_start_of_day() - (2 * sml::interval_time(now, profile));

            case CODE_PROFILE_1_MONTH: break;
            case CODE_PROFILE_1_YEAR: break;
            default:
                //  error
                BOOST_ASSERT_MSG(false, "not implemented yet");
                break;
            }

            return (now - backtrack).get_start_of_day();
        }

        std::string calculate_advance(sml_data const &first, sml_data const &second) {
            try {
                //
                //  see overloaded operator of sml_data
                //
                auto const s = second - first;
#ifdef __DEBUG
                std::stringstream ss;
                ss << '[' << second.reading_ << "-" << first.reading_ << "=" << s << ']';
                return ss.str();
#else
                return s;
#endif

            } catch (std::invalid_argument const &) {
            }
            return "0.0";
        }

        std::pair<cyng::obis, bool> convert_obis(cyng::obis const &code) {
            if (code == OBIS_REG_NEG_ACT_E) {
                //  Negative active energy(A+) total
                //  01 00 02 08 00 ff
                return {OBIS_PROFILE_POWER_NEG_ACTIVE, true};
            } else if (code == OBIS_REG_POS_ACT_E) {
                //  Positive active energy(A+), current value
                //  01 00 01 08 00 ff
                return {OBIS_PROFILE_POWER_POS_ACTIVE, true};
            } else if (code == OBIS_REG_HEAT_CURRENT) {
                //  Heat Energy (A), total, current value
                //  06 00 01 00 00 ff
                return {OBIS_PROFILE_HEAT_POS_OUTPUT, true};
            } else if (code == OBIS_WATER_CURRENT) {
                //  Water Volume (V), accumulated, total, current value
                //  08 00 01 00 00 ff
                return {OBIS_PROFILE_WATER_POS_OUTPUT, true};
            } else if (code == OBIS_REG_GAS_MC_1_0) {
                //  Gas Volume (meter), temperature converted (Vtc), forward, absolute, current value
                //  07 00 03 01 00 ff
                return {OBIS_PROFILE_GAS_POS_OUTPUT, true};
            }
            return {code, false};
        }

    } // namespace feed

} // namespace smf
