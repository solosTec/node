
#include <tasks/cleanup_db.h>

#include <smf/obis/db.h>
#include <smf/obis/profile.h>
#include <smf/report/utility.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    // cyng::channel_weak, cyng::controller &, cyng::logger
    cleanup_db::cleanup_db(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        cyng::logger logger, 
        cyng::db::session db, 
        cyng::obis profile, 
        std::chrono::hours max_age)
        : sigs_{
            std::bind(&cleanup_db::run, this), // start
            std::bind(&cleanup_db::vacuum, this), // vacuum
            std::bind(&cleanup_db::stop, this, std::placeholders::_1) // stop
        }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , db_(db)
        , profile_(profile)
        , max_age_(max_age) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"run", "vacuum"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    void cleanup_db::stop(cyng::eod) { CYNG_LOG_WARNING(logger_, "stop cleanup task"); }
    void cleanup_db::run() {

        auto const now = cyng::make_utc_date();
        auto sp = channel_.lock();
        BOOST_ASSERT_MSG(sp, "cleanup task already stopped");
        if (sp) {
            auto const interval = sml::interval_time(now, profile_);

            //
            //  cleanup
            //
            auto const count = smf::cleanup(db_, profile_, now - max_age_);
            CYNG_LOG_INFO(
                logger_,
                "[cleanup] " << count << " records older than " << max_age_.count() << "h from " << obis::get_name(profile_)
                             << " removed");

            //
            //  restart after half the interval time
            // don't start immediately and earliest after one hour
            //
            auto const delay = ((interval / 2) < std::chrono::hours(1)) ? std::chrono::hours(1) : (interval / 2);

            sp->suspend(
                delay,
                "run",
                //  handle dispatch errors
                std::bind(cyng::log_dispatch_error, logger_, std::placeholders::_1, std::placeholders::_2));
            CYNG_LOG_TRACE(logger_, "[cleanup] run " << obis::get_name(profile_) << " at " << now + interval);
        }
    }

    void cleanup_db::vacuum() { smf::vacuum(db_); }

} // namespace smf
