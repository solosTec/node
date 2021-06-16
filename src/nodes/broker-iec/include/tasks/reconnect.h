/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IEC_TASK_RECONNECT_H
#define SMF_IEC_TASK_RECONNECT_H

#include <db.h>

#include <smf/cluster/bus.h>
#include <smf/iec/parser.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/store/key.hpp>
#include <cyng/task/task_fwd.h>

#include <fstream>

namespace smf {

    class reconnect {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void()>,         //  run
            std::function<void(cyng::eod)> //  stop
            >;

      public:
        reconnect(cyng::channel_weak, cyng::logger, cyng::channel_weak);

        void stop(cyng::eod);

      private:
        void run();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;
        cyng::logger logger_;
        cyng::channel_weak client_;
    };

} // namespace smf

#endif
