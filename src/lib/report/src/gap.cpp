
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
#include <cyng/obj/intrinsics/date.h>

#include <fstream>
#include <iostream>

namespace smf {

    void generate_gap(
        cyng::db::session db,
        cyng::obis profile,
        std::filesystem::path root,
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
        //  reference date
        //
        // cyng::date prev = start - sml::reporting_period(profile, start);
        cyng::date prev = start;

        //
        //  data set
        //
        gap::readout_t data;

        loop_readout(db, profile, start, [&](srv_id_t id, cyng::date act_time) -> bool {
            //
            //
            //  calculate slot
            //
            auto const sr = sml::to_index(act_time, profile);
            BOOST_ASSERT(sr.second);
            auto const d = smf::sml::from_index_to_date(sr.first, profile);

#ifdef _DEBUG
            std::cout << "start: " << cyng::as_string(start) << ", d: " << cyng::as_string(d) << std::endl;
#endif

            if (sml::is_new_reporting_period(profile, prev, d)) {

                //
                //  detect missing report periods
                //
                auto const period_target = sml::reporting_period(profile, prev);
                auto const period_actual = d.sub<std::chrono::hours>(prev);
                auto const miss = (period_actual.count() / period_target.count());
                std::cout << ">> there are #" << miss << " missing report periods " << std::endl;
                for (int idx = 0; idx < miss; ++idx) {
                    auto const ts = prev + (period_target * idx);
                    auto const file_name = get_filename("gap-", profile, ts);
                    std::cout << ">> generate report " << root / file_name << std::endl;
                }

                if (!data.empty()) {
                    auto const file_name = get_filename("gap-", profile, prev);
                    std::cout << ">> generate report " << root / file_name << std::endl;
                    auto ofs = gap::open_report(root, file_name);
                    if (ofs.is_open()) {
                        gap::generate_report(ofs, profile, prev, data);
                        gap::clear_data(data);
                    }
                }
                //
                //  update time stamp
                //
                prev = d;
            }

            //
            //  update data set
            //
            auto pos = data.find(id);
            if (pos != data.end()) {
                std::cout << "> new slot #" << data.size() << ": " << sr.first << " of meter " << to_string(id) << std::endl;
                pos->second.insert(gap::make_slot(sr.first, d));
            } else {
                std::cout << "> new meter #" << data.size() << ": " << to_string(id) << std::endl;
                std::cout << "> new slot #0: " << sr.first << " of meter " << to_string(id) << std::endl;
                data.insert(gap::make_readout(id, gap::make_slot(sr.first, d)));
            }

            return true;
        });

        auto const file_name = get_filename("gap-", profile, prev);
        std::cout << ">> generate report " << root / file_name << std::endl;
        auto ofs = gap::open_report(root, file_name);
        if (ofs.is_open()) {
            gap::generate_report(ofs, profile, prev, data);
            gap::clear_data(data);
        }
    }

    namespace gap {
        void clear_data(readout_t &data_set) {
            for (auto &data : data_set) {
                std::cout << "> clear " << data.second.size() << " data records of meter " << to_string(data.first) << std::endl;
                data.second.clear();
            }
        }
        std::ofstream open_report(std::filesystem::path root, std::string file_name) {
            auto const file_path = root / file_name;
            return std::ofstream(file_path.string(), std::ios::trunc);
        }

        void generate_report(std::ofstream &ofs, cyng::obis profile, cyng::date const &d, readout_t const &data) {
            auto const [start, end] = sml::to_index_range(d, profile);
            BOOST_ASSERT(start <= end);
            std::cout << "> generate report for " << data.size() << " meters from " << start << " = "
                      << cyng::as_string(sml::from_index_to_date(start, profile), "%Y-%m-%d %H:%M:%S") << " to " << end << " = "
                      << cyng::as_string(sml::from_index_to_date(end, profile), "%Y-%m-%d %H:%M:%S") << std::endl;

            for (auto const &[id, slots] : data) {
                ofs << cyng::as_string(d, "%Y-%m-%d;");
                ofs << to_string(id);
                for (auto idx = start; idx < end; ++idx) {
                    ofs << ';';
                    auto const pos = slots.find(idx);
                    if (pos != slots.end()) {
                        ofs << 'Y';
                    } else {
                        ofs << 'N';
                    }
                }
                ofs << std::endl;
            }
        }
    } // namespace gap

} // namespace smf
