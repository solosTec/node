/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_PUSH_H
#define SMF_SEGW_TASK_PUSH_H

#include <cfg.h>

#include <cyng/log/logger.h>
#include <cyng/store/key.hpp>
#include <cyng/task/task_fwd.h>

namespace smf {

    namespace ipt {
        class bus;
    }

    /**
     * control push operations
     */
    class push {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(void)>, //	run
            std::function<void(cyng::eod)>>;

      public:
        push(cyng::channel_weak, cyng::logger, ipt::bus &, cfg &config, cyng::key_t);

      private:
        void stop(cyng::eod);
        void run();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;

        /**
         * global logger
         */
        cyng::logger logger_;

        ipt::bus &bus_;
        cfg &cfg_;
        cyng::key_t const key_;
    };

} // namespace smf

#endif
