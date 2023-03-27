/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_BUS_WATCHDOG_H
#define SMF_IPT_BUS_WATCHDOG_H

#include <cyng/log/logger.h>
#include <cyng/log/record.h>
#include <cyng/task/channel.h>
#include <cyng/task/task_fwd.h>

#include <iostream>

namespace smf {
    namespace ipt {

        class bus;
        /**
         * IP-T watchdog
         */
        class watchdog {
            template <typename U> friend class cyng::task;

            using signatures_t = std::tuple<
                std::function<void(std::chrono::minutes)>, // timeout
                std::function<void(cyng::eod)>             // stop
                >;

          public:
            watchdog(cyng::channel_weak wp, cyng::logger logger, bus &);
            ~watchdog() = default;

            void stop(cyng::eod) { CYNG_LOG_INFO(logger_, "[watchdog] stop"); }

          private:
            void timeout(std::chrono::minutes);

          private:
            signatures_t sigs_;
            cyng::channel_weak channel_;
            cyng::logger logger_;
            bus &bus_;
        };
    } // namespace ipt
} // namespace smf

#endif
