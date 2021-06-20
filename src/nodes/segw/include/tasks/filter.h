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
            std::function<void(cyng::eod)>,
            std::function<void(cyng::buffer_t)>,
            std::function<void(std::string)>, //	reset_target_channels
            std::function<void(void)>         //	update_statistics
            >;

      public:
        filter(cyng::channel_weak, cyng::controller &ctl, cyng::logger, cfg &, lmn_type);

      private:
        void stop(cyng::eod);

        /**
         * incoming raw data from serial interface
         */
        void receive(cyng::buffer_t);
        void reset_target_channels(std::string);
        void update_statistics();
        void flash_led(std::chrono::milliseconds, std::size_t);

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
        cfg_gpio cfg_gpio_;

        /**
         * parser for wireless M-Bus data
         */
        mbus::radio::parser parser_;

        /**
         * broker list
         */
        std::vector<cyng::channel_ptr> targets_;

        std::map<std::string, std::chrono::system_clock::time_point> access_times_;
        std::size_t accumulated_bytes_;
    };
} // namespace smf

#endif
