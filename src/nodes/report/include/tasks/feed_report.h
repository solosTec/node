/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_REPORT_TASK_FEED_H
#define SMF_REPORT_TASK_DEEF_H

#include <smf/mbus/server_id.h>

#include <cyng/db/session.h>
#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/task/task_fwd.h>

namespace smf {

    class feed_report {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(void)>,     // start
            std::function<void(cyng::eod)> // stop
            >;

      public:
        feed_report(
            cyng::channel_weak,
            cyng::controller &,
            cyng::logger,
            cyng::db::session,
            cyng::obis profile,
            cyng::obis_path_t filter,
            std::string path,
            std::chrono::hours backtrack,
            std::string prefix,
            bool print_version,
            bool debug_mode,
            bool customer);

        ~feed_report() = default;

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
        cyng::obis_path_t const filter_;
        std::filesystem::path const root_;
        std::chrono::hours const backtrack_;
        std::string const prefix_;
        bool const print_version_;
        bool const debug_mode_;
        bool const customer_;
    };

} // namespace smf

#endif
