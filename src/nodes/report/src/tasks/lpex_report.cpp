
#include <tasks/lpex_report.h>

#include <smf/obis/db.h>
#include <smf/obis/profile.h>
// #include <smf/report/csv.h>
#include <smf/report/lpex.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    // cyng::channel_weak, cyng::controller &, cyng::logger
    lpex_report::lpex_report(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        cyng::logger logger, 
        cyng::db::session db, 
        cyng::obis profile, 
        cyng::obis_path_t filter,
        std::string path,
        std::chrono::hours backtrack,
        std::string prefix,
        bool print_version,
        bool separated,
        bool debug_mode)
        : sigs_{
            std::bind(&lpex_report::run, this), // start
            std::bind(&lpex_report::stop, this, std::placeholders::_1) // stop
        }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , db_(db)
        , profile_(profile)
        , filter_(filter)
        , root_(path)
        , backtrack_(backtrack)
        , prefix_(prefix)
        , print_version_(print_version)
        , separated_(separated)
        , debug_mode_(debug_mode) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"run"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    void lpex_report::stop(cyng::eod) { CYNG_LOG_WARNING(logger_, "stop report task"); }
    void lpex_report::run() {

        auto const now = std::chrono::system_clock::now();
        auto sp = channel_.lock();
        BOOST_ASSERT_MSG(sp, "report task already stopped");
        if (sp) {
            auto const next = now + sml::interval_time(now, profile_);
            BOOST_ASSERT_MSG(next > now, "negative time span");

            //
            //  restart
            //
            auto const span = std::chrono::duration_cast<std::chrono::seconds>(next - now);
            sp->suspend(span, "run");
            CYNG_LOG_TRACE(logger_, "[report LPex] run " << obis::get_name(profile_) << " at " << next);

            //
            //  generate report
            //
            // generate_csv(db_, profile_, root_, backtrack_, now, prefix_);
            generate_lpex(
                db_,
                profile_,
                filter_,
                root_,
                backtrack_,
                now,
                prefix_,
                print_version_,
                separated_,
                debug_mode_,
                [=, this](
                    std::chrono::system_clock::time_point start, std::chrono::minutes range, std::size_t count, std::size_t size) {
                    auto const d = cyng::date::make_date_from_local_time(start);

                    CYNG_LOG_TRACE(
                        logger_,
                        "[LPEx report] scan " << obis::get_name(profile_) << " from " << cyng::as_string(d, "%F %T") << " + "
                                              << range.count() << " minutes and found " << count << " meters");
                });
        }
    }
} // namespace smf
