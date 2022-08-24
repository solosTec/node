/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_REPORT_TASK_CSV_REPORT_H
#define SMF_REPORT_TASK_CSV_REPORT_H

#include <smf/mbus/server_id.h>

#include <cyng/db/session.h>
#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/task/task_fwd.h>

namespace smf {

    class csv_report {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(void)>,     // start
            std::function<void(cyng::eod)> // stop
            >;

      public:
        csv_report(
            cyng::channel_weak,
            cyng::controller &,
            cyng::logger,
            cyng::db::session,
            cyng::obis profile,
            std::string path,
            std::chrono::hours backtrack,
            std::string prefix);

        ~csv_report() = default;

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
        std::string const prefix_;
    };

} // namespace smf

#endif
