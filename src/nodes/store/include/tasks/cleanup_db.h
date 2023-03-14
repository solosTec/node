/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_REPORT_TASK_CLEANUP_DB_H
#define SMF_REPORT_TASK_CLEANUP_DB_H

#include <cyng/db/session.h>
#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/task/task_fwd.h>

namespace smf {

    /**
     * Remove all outdated data records.
     */
    class cleanup_db {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void()>,         // start
            std::function<void()>,         // vacuum
            std::function<void(cyng::eod)> // stop
            >;

      public:
        cleanup_db(
            cyng::channel_weak wp,
            cyng::controller &ctl,
            cyng::logger logger,
            cyng::db::session db,
            cyng::obis profile,
            std::chrono::hours max_age);

        ~cleanup_db() = default;

      private:
        void stop(cyng::eod);
        void run();
        void vacuum();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_; //	self
        cyng::controller &ctl_;
        cyng::logger logger_;
        cyng::db::session db_;
        cyng::obis const profile_;
        std::chrono::hours const max_age_;
    };

} // namespace smf

#endif
