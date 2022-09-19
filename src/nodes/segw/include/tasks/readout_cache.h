/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_READOUT_CACHE_H
#define SMF_SEGW_READOUT_CACHE_H

#include <cfg.h>
#include <config/cfg_sml.h>

#include <smf/mbus/radio/parser.h>

#include <cyng/log/logger.h>
#include <cyng/task/task_fwd.h>

namespace smf {
    /**
     * Processor for wireless M-Bus data (EN 13757-4).
     *
     * - decode data - if AES key available
     * - build up load profile
     * - update "meter" table
     */
    class readout_cache {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(cyng::buffer_t)>,
            std::function<void(void)>,        //    reset_target_channels
            std::function<void(std::string)>, //	add_target_channel
            std::function<void(cyng::eod)>    //    stop()
            >;

      public:
        readout_cache(cyng::channel_weak, cyng::controller &ctl, cyng::logger, cfg &);

      private:
        void stop(cyng::eod);

        /**
         * Incoming raw wireless M-Bus data from serial interface
         * "receive". The data comes from the "filter@X" task, so not all
         * devices will be visible here.
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
         * read wireless M-Bus data header
         */
        void decode(mbus::radio::header const &h, mbus::radio::tplayer const &t, cyng::buffer_t const &data);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;

        cyng::controller &ctl_;

        /**
         * global logger
         */
        cyng::logger logger_;

        /**
         * config/data cache
         */
        cfg &cfg_;
        cfg_sml const cfg_sml_;

        /**
         * parser for wireless M-Bus data
         */
        mbus::radio::parser parser_;
    };
} // namespace smf

#endif
