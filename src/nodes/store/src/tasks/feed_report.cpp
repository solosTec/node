
#include <tasks/feed_report.h>

#include <smf/obis/db.h>
#include <smf/obis/profile.h>
#include <smf/report/feed.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    // cyng::channel_weak, cyng::controller &, cyng::logger
    feed_report::feed_report(
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
        bool debug_mode,
        bool customer)
        : sigs_{
            std::bind(&feed_report::run, this), // start
            std::bind(&feed_report::stop, this, std::placeholders::_1) // stop
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
        , debug_mode_(debug_mode)
        , customer_(customer) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"run"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    void feed_report::stop(cyng::eod) { CYNG_LOG_WARNING(logger_, "stop feed report task"); }
    void feed_report::run() {

        //
        //  get reference time
        //
        auto const now = cyng::make_utc_date();

        auto sp = channel_.lock();
        BOOST_ASSERT_MSG(sp, "feed report task already stopped");
        if (sp) {
            //  make sure that the interval is at least 6h long
            auto interval = sml::interval_time(now, profile_);
            if (interval < std::chrono::minutes(6 * 60)) {
                interval = std::chrono::minutes(6 * 60);
            }
            auto const next = now + interval;
            BOOST_ASSERT_MSG(next > now, "negative time span");

            //
            //  restart
            //
            //  handle dispatch errors
            sp->suspend(
                interval, "run", std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2));
            CYNG_LOG_TRACE(logger_, "[feed report] run " << obis::get_name(profile_) << " at " << cyng::as_string(next, "%F %T"));

            //
            //  generate report
            //
            smf::generate_feed(db_, profile_, filter_, root_, prefix_, now, backtrack_, print_version_, debug_mode_, customer_);
        }
    }
} // namespace smf
