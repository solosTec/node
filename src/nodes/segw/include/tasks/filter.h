/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_TASK_FILTER_H
#define SMF_SEGW_TASK_FILTER_H

#include <cfg.h>
#include <config/cfg_blocklist.h>
#include <config/cfg_gpio.h>
#include <config/cfg_lmn.h>

#include <smf/mbus/radio/parser.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/task/task_fwd.h>

namespace smf {
    /**
     * filter for wireless M-Bus data
     */
    class filter {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(cyng::buffer_t)>,
            std::function<void(void)>,        //	reset_target_channels
            std::function<void(std::string)>, //	add_target_channel
            std::function<void(void)>,        //	update_statistics
            std::function<void(cyng::eod)>>;

        struct access_data {
            access_data();
            /**
             * in percent
             */
            std::string calculate_rate() const;

            std::chrono::system_clock::time_point access_;
            std::size_t total_;
            std::size_t dropped_;
        };

      public:
        filter(cyng::channel_weak, cyng::controller &ctl, cyng::logger, cfg &, lmn_type);

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

        void flash_led(std::chrono::milliseconds, std::size_t);

        /**
         * Apply filter and forward all valid data.
         */
        void check(mbus::radio::header const &h, mbus::radio::tpl const &t, cyng::buffer_t const &data);
        bool check_frequency(std::string const &id);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;

        cyng::controller &ctl_;

        /**
         * global logger
         */
        cyng::logger logger_;

        /**
         * blocklist with IP addresses
         */
        cfg_blocklist cfg_blocklist_;

        /**
         * GPIO configuration
         */
        cfg_gpio cfg_gpio_;

        /**
         * parser for wireless M-Bus data
         */
        mbus::radio::parser parser_;

        /**
         * broker list
         */
        std::set<std::string> targets_;

        /**
         * simple statistics calculation
         */
        std::map<std::string, access_data> access_times_;

        /**
         * accumulated bytes since last statics update
         */
        std::size_t accumulated_bytes_;
    };
} // namespace smf

#endif
