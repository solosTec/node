/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_EN13757_H
#define SMF_SEGW_EN13757_H

#include <cfg.h>

//#include <smf/mbus/radio/parser.h>

#include <cyng/log/logger.h>
//#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/task/task_fwd.h>

namespace smf {
    /**
     * Processor for wireless M-Bus data (EN 13757-4).
     *
     * - decode data - if AES key available
     * - build up load profile
     * - update "meter" table
     */
    class en13757 {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(cyng::buffer_t)>,
            std::function<void(void)>,        //    reset_target_channels
            std::function<void(std::string)>, //	add_target_channel
            std::function<void(void)>,        //	update_statistics
            std::function<void(cyng::eod)>    //    stop()
            >;

      public:
        en13757(cyng::channel_weak, cyng::controller &ctl, cyng::logger, cfg &);

      private:
        void stop(cyng::eod);

        /**
         * incoming raw wireless M-Bus data from serial interface
         * "receive"
         */
        void receive(cyng::buffer_t);

        /** @"reset-data-sinks"
         *
         * Remove all data sinks
         */
        void reset_target_channels();

        /** @"add-data-sink"
         *
         * Add a new listener task
         */
        void add_target_channel(std::string);

        /**
         * "update-statistics"
         */
        void update_statistics();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;

        cyng::controller &ctl_;

        /**
         * global logger
         */
        cyng::logger logger_;
    };
} // namespace smf

#endif
