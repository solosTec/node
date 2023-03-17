
#include <tasks/gap_report.h>

#include <smf/obis/db.h>
#include <smf/obis/profile.h>
#include <smf/report/gap.h>
#include <smf/report/utility.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    gap_report::gap_report(
        cyng::channel_weak wp,
        cyng::controller& ctl,
        cyng::logger logger,
        cyng::db::session db,
        cyng::obis profile,
        std::string path,
        std::chrono::hours max_age)
        : sigs_{
            std::bind(&gap_report::run, this, std::placeholders::_1), // start
            std::bind(&gap_report::stop, this, std::placeholders::_1) // stop
        }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , db_(db)
        , profile_(profile)
        , root_(path)
        , max_age_(max_age) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"run"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    void gap_report::stop(cyng::eod) { CYNG_LOG_WARNING(logger_, "stop gap report task"); }
    void gap_report::run(std::chrono::hours span) {

        //
        //  get reference time
        //
        auto const now = cyng::make_utc_date();

        auto sp = channel_.lock();
        BOOST_ASSERT_MSG(sp, "gap report task already stopped");
        if (sp) {
            auto const interval = sml::interval_time(now, profile_);
            auto const next = now + interval;
            BOOST_ASSERT_MSG(next > now, "negative time span");

            //
            //  restart
            //
            //  handle dispatch errors
            sp->suspend(
                span, "run", std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2), span);
            CYNG_LOG_TRACE(logger_, "[gap report] run " << obis::get_name(profile_) << " at " << next);

            //
            //  report
            //
            smf::generate_gap(db_, profile_, root_, now, max_age_);
        }
    }
} // namespace smf
