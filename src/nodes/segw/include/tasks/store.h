/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_STORE_H
#define SMF_SEGW_TASK_STORE_H

#include <cfg.h>

#include <cyng/log/logger.h>
#include <cyng/store/key.hpp>
#include <cyng/task/task_fwd.h>

namespace smf {

    namespace ipt {
        class bus;
    }

    /**
     * Transfer data from data cache ("readout", "readoutData") to data mirror
     */
    class store {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(void)>, //	init
            std::function<void(void)>, //	run
            std::function<void(cyng::eod)>>;

      public:
        store(cyng::channel_weak, cyng::logger, ipt::bus &, cfg &config, cyng::obis);

      private:
        void stop(cyng::eod);
        void init();
        void run();

        /**
         * @return count of processed readouts and total count of all readout records
         */
        std::pair<std::size_t, std::size_t> transfer(std::chrono::system_clock::time_point);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;

        /**
         * global logger
         */
        cyng::logger logger_;

        ipt::bus &bus_;
        cfg &cfg_;
        cyng::obis const profile_;
    };

} // namespace smf

#endif
