/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_REPORT_TASK_REPORT_H
#define SMF_REPORT_TASK_REPORT_H

#include <cyng/db/session.h>
#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/task/task_fwd.h>
//#include <cyng/vm/mesh.h>
//
//#include <functional>
//#include <memory>
//#include <string>
//#include <tuple>
//
//#include <boost/uuid/uuid.hpp>

namespace smf {

    class report {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(void)>,     // start
            std::function<void(cyng::eod)> // stop
            >;

      public:
        report(
            cyng::channel_weak,
            cyng::controller &,
            cyng::logger,
            cyng::db::session,
            cyng::obis profile,
            std::string path,
            std::chrono::hours backtrack);

        ~report() = default;

      private:
        void stop(cyng::eod);
        void run();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_; //	self
        cyng::controller &ctl_;
        cyng::logger logger_;
        cyng::db::session db_;
        cyng::obis const profile_;
        std::filesystem::path const root_;
        std::chrono::hours const backtrack_;
    };

    void generate_report(
        cyng::db::session,
        cyng::obis profile,
        std::filesystem::path,
        std::chrono::hours backtrack,
        std::chrono::system_clock::time_point);
    void generate_report_1_minute(cyng::db::session, std::chrono::system_clock::time_point);
    void generate_report_15_minutes(cyng::db::session, std::chrono::system_clock::time_point);
    void generate_report_60_minutes(cyng::db::session, std::chrono::system_clock::time_point);
    void generate_report_24_hour(cyng::db::session, std::chrono::system_clock::time_point);
    void generate_report_1_month(cyng::db::session, std::chrono::system_clock::time_point);
    void generate_report_1_year(cyng::db::session, std::chrono::system_clock::time_point);

} // namespace smf

#endif
