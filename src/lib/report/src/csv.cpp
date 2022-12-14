
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
#include <cyng/obj/util.hpp>

#include <fstream>
#include <iostream>

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace smf {

    void generate_csv(
        cyng::db::session db,
        cyng::obis profile,
        std::filesystem::path root,
        std::string prefix,
        cyng::date now,
        std::chrono::hours backtrack) {

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
                            csv::generate_report(profile, prev, data_set, root, prefix);
                            data::clear(data_set);
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
                data::update(data_set, id, reg, sr.first, code, scaler, mbus::to_u8(unit), value, status);

                return true;
            });

        //
        //  generate report for last data set
        //
        if (!data_set.empty()) {
            csv::generate_report(profile, prev, data_set, root, prefix);
            data::clear(data_set);
        }
    }

    namespace csv {
        std::ofstream open_report(std::filesystem::path root, std::string file_name) {
            auto const file_path = root / file_name;
            std::ofstream ofs(file_path.string(), std::ios::trunc);

            if (ofs.is_open()) {

                //
                // header
                //
            }

            return ofs; // move
        }

        void generate_report(
            cyng::obis profile,
            cyng::date const &d,
            data::data_set_t const &data_set,
            std::filesystem::path root,
            std::string prefix) {

            auto const [start, end] = sml::to_index_range(d, profile);
            BOOST_ASSERT(start <= end);
            std::cout << "> generate report for " << data_set.size() << " meters from " << start << " = "
                      << cyng::as_string(sml::from_index_to_date(start, profile), "%Y-%m-%d %H:%M:%S") << " to " << end << " = "
                      << cyng::as_string(sml::from_index_to_date(end, profile), "%Y-%m-%d %H:%M:%S") << std::endl;

            //
            //  meter -> register -> slot -> data
            //
            for (auto const &[id, values] : data_set) {
                std::cout << to_string(id) << " has " << values.size() << " entries" << std::endl;
                std::cout << '#' << to_string(id) << std::endl;
                // ofs << '#' << to_string(id) << std::endl;

                auto const file_name = get_filename(prefix, profile, id, d);
                std::cout << ">> generate report " << root / file_name << std::endl;
                auto ofs = csv::open_report(root, file_name);
                if (ofs.is_open()) {

                    //
                    //  header
                    //
                    ofs << "date;time;id";
                    for (auto const &[reg, data] : values) {
                        ofs << ';' << cyng::to_string(reg);
                    }
                    ofs << std::endl;

                    //
                    //  body
                    //

                    for (auto idx = start; idx < end; ++idx) {
                        ofs << cyng::as_string(sml::from_index_to_date(idx, profile), "%Y-%m-%d;%H:%M;");
                        ofs << to_string(id);

                        for (auto const &[reg, data] : values) {
                            ofs << ';';
                            auto const pos = data.find(idx);
                            if (pos != data.end()) {
                                ofs << pos->second.reading_;
                            } else {
                                ofs << "[-]";
                            }
                        }
                        ofs << std::endl;
                    }
                    ofs.close();
                }
            }
        }
    } // namespace csv
} // namespace smf
