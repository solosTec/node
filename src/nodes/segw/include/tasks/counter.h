/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_COUNTER_H
#define SMF_SEGW_TASK_COUNTER_H

#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>

#include <filesystem>

namespace smf {

    class cfg; //  contains operation time counter

    /**
     * Counter for operating time in seconds
     */
    class counter {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(void)>,     //	inc
            std::function<void(void)>,     //	save
            std::function<void(cyng::eod)> // stop task
            >;

      public:
        counter(cyng::channel_weak, cyng::logger, cfg &);

      private:
        void stop(cyng::eod);

        /**
         * @brief increment by one second
         *
         */
        void inc();

        /**
         * @brief save current counter
         *
         * This function is not thread safe.
         */
        void save();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;

        /**
         * global logger
         */
        cyng::logger logger_;

        /**
         * contains operation time counter
         */
        cfg &cfg_;
    };

} // namespace smf

#endif
