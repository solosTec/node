
#include <tasks/report.h>

#include <smf/config/schemes.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/db/storage.h>
#include <cyng/log/record.h>
#include <cyng/obj/util.hpp>
#include <cyng/sql/dsl.h>
#include <cyng/sql/dsl/placeholder.h>
#include <cyng/sql/sql.hpp>
#include <cyng/task/channel.h>

#include <iostream>

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
            sp->set_channel_names({"connect", "run"});
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

            auto const span = std::chrono::duration_cast<std::chrono::seconds>(next - now);
            sp->suspend(span, "run");

            CYNG_LOG_TRACE(logger_, "[report] run " << obis::get_name(profile_) << " at " << next);

            generate_report(db_, profile_, root_, backtrack_, now);
        }
    }

    void generate_report(
        cyng::db::session db,
        cyng::obis profile,
        std::filesystem::path root,
        std::chrono::hours backtrack,
        std::chrono::system_clock::time_point now) {

#ifdef __DEBUG
        auto const ms = config::get_table_sml_readout();
        // auto const sql =
        //     cyng::sql::select(db_.get_dialect(), m).all(m, false).from().where(cyng::sql::make_constant("meterID") ==
        //     profile_)();
        std::string const sql = "SELECT " SELECT tag, meterID, profile, trx, status, datetime(actTime),
                          datetime(received) FROM TSMLreadout WHERE profile = ? ";
                                                                                auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            stmt->push(cyng::make_object(profile), 0); //	profile
            while (auto res = stmt->get_result()) {
                auto const rec = cyng::to_record(ms, res);
                //  example
                //  <tag: a2e26a80-fe36-45dc-8a47-1918616243ef>
                //  [meterID: 01a815743145040102]
                //  [profile: 8181c78611ff]
                //  [trx: 44174435]
                //  [status: 000202a0]
                //  [actTime: 2458-12-31T00:00:00+0100]
                //  [received: 2458-12-31T00:00:00+0100]
                std::cout << rec.to_string() << std::endl;
                // CYNG_LOG_TRACE(logger_, "[report] run " << obis::get_name(profile) << ": " << rec.to_string());
            }
        }
#endif
        switch (profile.to_uint64()) {
        case CODE_PROFILE_1_MINUTE: generate_report_1_minute(db, backtrack, now); break;
        case CODE_PROFILE_15_MINUTE: generate_report_15_minutes(db, backtrack, now); break;
        case CODE_PROFILE_60_MINUTE: generate_report_60_minutes(db, backtrack, now); break;
        case CODE_PROFILE_24_HOUR: generate_report_24_hour(db, backtrack, now); break;
        case CODE_PROFILE_1_MONTH: generate_report_1_month(db, backtrack, now); break;
        case CODE_PROFILE_1_YEAR: generate_report_1_year(db, backtrack, now); break;

        default: break;
        }
    }

    void generate_report_1_minute(cyng::db::session db, std::chrono::hours backtrack, std::chrono::system_clock::time_point now) {
        auto constexpr profile = CODE_PROFILE_1_MINUTE;
    }

    void generate_report_15_minutes(cyng::db::session db, std::chrono::hours backtrack, std::chrono::system_clock::time_point now) {

        auto constexpr profile = CODE_PROFILE_15_MINUTE;

        //
        //  generate a report about the previous day
        //

        //  * get start and end time of previous day
        //  * collect all occurring register values in this range

        auto const ms = config::get_table_sml_readout();
        std::string const sql =
            "SELECT tag, gen, meterID, profile, trx, status, datetime(actTime), datetime(received) FROM TSMLreadout WHERE profile = ? AND received > julianday(?) ORDER BY received";
        auto stmt = db.create_statement();
        std::pair<int, bool> const r = stmt->prepare(sql);
        if (r.second) {
            auto const start = now - backtrack;
            std::cout << "start: " << start << std::endl;
            stmt->push(cyng::make_object(profile), 0); //	profile
            stmt->push(cyng::make_object(start), 0);   //	start time
            while (auto res = stmt->get_result()) {
                auto const rec = cyng::to_record(ms, res);
                //  example
                //  <tag: a2e26a80-fe36-45dc-8a47-1918616243ef>
                //  [meterID: 01a815743145040102]
                //  [profile: 8181c78611ff]
                //  [trx: 44174435]
                //  [status: 000202a0]
                //  [actTime: 2458-12-31T00:00:00+0100]
                //  [received: 2458-12-31T00:00:00+0100]
                std::cout << rec.to_string() << std::endl;
                // CYNG_LOG_TRACE(logger_, "[report] run " << obis::get_name(profile) << ": " << rec.to_string());
            }
        }
    }

    void generate_report_60_minutes(cyng::db::session db, std::chrono::hours backtrack, std::chrono::system_clock::time_point now) {
        auto constexpr profile = CODE_PROFILE_60_MINUTE;
    }

    void generate_report_24_hour(cyng::db::session db, std::chrono::hours backtrack, std::chrono::system_clock::time_point now) {
        auto constexpr profile = CODE_PROFILE_24_HOUR;
    }
    void generate_report_1_month(cyng::db::session db, std::chrono::hours backtrack, std::chrono::system_clock::time_point now) {
        auto constexpr profile = CODE_PROFILE_1_MONTH;
    }
    void generate_report_1_year(cyng::db::session db, std::chrono::hours backtrack, std::chrono::system_clock::time_point now) {
        auto constexpr profile = CODE_PROFILE_1_YEAR;
    }

} // namespace smf
