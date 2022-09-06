
#include <tasks/csv_report.h>

#include <smf/obis/db.h>
#include <smf/obis/profile.h>
#include <smf/report/csv.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    // cyng::channel_weak, cyng::controller &, cyng::logger
    csv_report::csv_report(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        cyng::logger logger, 
        cyng::db::session db, 
        cyng::obis profile, 
        std::string path,
        std::chrono::hours backtrack,
        std::string prefix)
        : sigs_{
            std::bind(&csv_report::run, this), // start
            std::bind(&csv_report::stop, this, std::placeholders::_1) // stop
        }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , db_(db)
        , profile_(profile)
        , root_(path)
        , backtrack_(backtrack)
        , prefix_(prefix) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"run"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    void csv_report::stop(cyng::eod) { CYNG_LOG_WARNING(logger_, "stop CSV report task"); }
    void csv_report::run() {

        auto const now = std::chrono::system_clock::now();
        auto sp = channel_.lock();
        BOOST_ASSERT_MSG(sp, "CSV report task already stopped");
        if (sp) {
            auto const next = now + sml::interval_time(now, profile_);
            BOOST_ASSERT_MSG(next > now, "negative time span");

            //
            //  restart
            //
            auto const span = std::chrono::duration_cast<std::chrono::seconds>(next - now);
            sp->suspend(span, "run");
            CYNG_LOG_TRACE(logger_, "[CSV report] run " << obis::get_name(profile_) << " at " << next);

            //
            //  generate report
            //
            smf::generate_csv(db_, profile_, root_, backtrack_, now, "");
        }
    }
} // namespace smf
