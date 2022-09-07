
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
        std::chrono::hours max_age,
        std::size_t limit)
        : sigs_{
            std::bind(&cleanup_db::run, this, std::placeholders::_1), // start
            std::bind(&cleanup_db::stop, this, std::placeholders::_1) // stop
        }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , db_(db)
        , profile_(profile)
        , max_age_(max_age)
        , limit_(limit < 64 ? 64 : limit) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"run"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    void cleanup_db::stop(cyng::eod) { CYNG_LOG_WARNING(logger_, "stop cleanup task"); }
    void cleanup_db::run(std::chrono::hours span) {

        auto const now = std::chrono::system_clock::now();
        auto sp = channel_.lock();
        BOOST_ASSERT_MSG(sp, "cleanup task already stopped");
        if (sp) {
            auto const next = now + sml::interval_time(now, profile_);
            BOOST_ASSERT_MSG(next > now, "negative time span");

            //
            //  cleanup
            //
            auto const size = smf::cleanup(db_, profile_, now - max_age_, limit_);
            CYNG_LOG_INFO(
                logger_,
                "[cleanup] " << size << " records older than " << max_age_.count() << "h from " << obis::get_name(profile_)
                             << " removed");

            //
            //  restart
            //
            sp->suspend(span, "run", span);
            CYNG_LOG_TRACE(logger_, "[cleanup] run " << obis::get_name(profile_) << " at " << next);
        }
    }
} // namespace smf
