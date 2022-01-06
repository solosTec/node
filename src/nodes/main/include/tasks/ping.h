/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MAIN_TASK_PING_H
#define SMF_MAIN_TASK_PING_H

#include <db.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/task/controller.h>
#include <cyng/task/task_fwd.h>

#include <boost/uuid/uuid.hpp>

namespace smf {

    // class session;
    class ping {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void()>,         //   update
            std::function<void(cyng::eod)> //  stop
            >;

      public:
        ping(cyng::channel_weak, cyng::controller &ctl, cyng::logger logger, db &, std::function<void()>);
        ~ping();

      private:
        void stop(cyng::eod);
        void update();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::logger logger_;
        db &cache_;
        std::function<void()> trigger_;
    };

} // namespace smf

#endif
