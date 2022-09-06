/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_REPORT_TASK_GAP_H
#define SMF_REPORT_TASK_GAP_H

#include <cyng/db/session.h>
#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/task/task_fwd.h>

namespace smf {

    /**
     * Remove all outdated data records.
     */
    class gap_report {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(std::chrono::hours)>, // start
            std::function<void(cyng::eod)>           // stop
            >;

      public:
        gap_report(
            cyng::channel_weak wp,
            cyng::controller &ctl,
            cyng::logger logger,
            cyng::db::session db,
            cyng::obis profile,
            std::string path,
            std::chrono::hours max_age,
            std::chrono::minutes utc_offset);

        ~gap_report() = default;

      private:
        void stop(cyng::eod);
        void run(std::chrono::hours);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_; //	self
        cyng::controller &ctl_;
        cyng::logger logger_;
        cyng::db::session db_;
        cyng::obis const profile_;
        std::filesystem::path root_;
        std::chrono::hours const max_age_;
        std::chrono::minutes const utc_offset_;
    };

} // namespace smf

#endif
