
#include <tasks/report.h>

//#include <smf/config/schemes.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>
#include <smf/report/report.h>

#include <cyng/db/storage.h>
#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/sql/dsl.h>
#include <cyng/sql/dsl/placeholder.h>
#include <cyng/sql/sql.hpp>
#include <cyng/sys/clock.h>
#include <cyng/task/channel.h>

#include <iostream>

#include <boost/uuid/nil_generator.hpp>

namespace smf {

    // cyng::channel_weak, cyng::controller &, cyng::logger
    report::report(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        cyng::logger logger, 
        cyng::db::session db, 
        cyng::obis profile, 
        std::string path,
        std::chrono::hours backtrack)
        : sigs_{
            std::bind(&report::run, this), // start
            std::bind(&report::stop, this, std::placeholders::_1) // stop
        }
        , channel_(wp)
        , ctl_(ctl)
        , logger_(logger)
        , db_(db)
        , profile_(profile)
        , root_(path)
        , backtrack_(backtrack) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"run"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }
    }

    void report::stop(cyng::eod) { CYNG_LOG_WARNING(logger_, "stop report task"); }
    void report::run() {

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
            CYNG_LOG_TRACE(logger_, "[report] run " << obis::get_name(profile_) << " at " << next);

            //
            //  generate report
            //
            generate_report(db_, profile_, root_, backtrack_, now);
        }
    }
} // namespace smf
