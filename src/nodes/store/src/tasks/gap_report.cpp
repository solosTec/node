
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
        std::chrono::hours max_age,
        std::chrono::minutes utc_offset)
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
        , max_age_(max_age)
        , utc_offset_(utc_offset) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"run"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    void gap_report::stop(cyng::eod) { CYNG_LOG_WARNING(logger_, "stop gap report task"); }
    void gap_report::run(std::chrono::hours span) {

        auto const now = std::chrono::system_clock::now();
        auto sp = channel_.lock();
        BOOST_ASSERT_MSG(sp, "gap report task already stopped");
        if (sp) {
            auto const next = now + sml::interval_time(now, profile_);
            BOOST_ASSERT_MSG(next > now, "negative time span");

            //
            //  restart
            //
            sp->suspend(span, "run", span);
            CYNG_LOG_TRACE(logger_, "[gap report] run " << obis::get_name(profile_) << " at " << next);

            //
            //  report
            //
            smf::generate_gap(db_, profile_, root_, std::chrono::hours(1), now - max_age_, utc_offset_);
            // generate_lpex(db_, profile_, root_, backtrack_, now, prefix_, utc_offset_, print_version_);
        }
    }
} // namespace smf
