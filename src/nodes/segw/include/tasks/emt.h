/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_SMGW_TASK_EMT_H
#define SMF_SMGW_TASK_EMT_H

#include <cfg.h>
#include <config/cfg_http_post.h>
#include <config/cfg_sml.h>

#include <smf/mbus/radio/parser.h>

#include <cyng/db/session.h>
#include <cyng/log/logger.h>
#include <cyng/net/http_client_factory.h>
#include <cyng/task/task_fwd.h>

namespace smf {
    /**
     * Processor for wireless M-Bus data (EMT).
     *
     */
    class emt {
        template <typename T> friend class cyng::task;

        using signatures_t = std::tuple<
            std::function<void(cyng::buffer_t)>,
            std::function<void(void)>,        // reset_target_channels
            std::function<void(std::string)>, // add_target_channel
            std::function<void()>,            // push
            std::function<void()>,            // backup
            std::function<void(cyng::eod)>    // stop()
            >;

      public:
        emt(cyng::channel_weak, cyng::controller &ctl, cyng::logger, cyng::db::session, cfg &);

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

        void update_cache(mbus::radio::header const &h, mbus::radio::tplayer const &tpl, cyng::buffer_t const &data);
        // void update_load_profile(mbus::radio::header const &h, mbus::radio::tplayer const &tpl, cyng::buffer_t const &data);

        void decode(mbus::radio::header const &h, mbus::radio::tplayer const &tpl, cyng::buffer_t const &data);

        /**
         * send data and clear cache
         */
        void push();
        void push_data(cyng::channel_ptr);

        void backup();

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;

        cyng::controller &ctl_;
        cyng::net::http_client_factory client_factory_;
        cyng::net::http_client_proxy proxy_; //!< holds reference

        /**
         * global logger
         */
        cyng::logger logger_;

        /**
         * sql database
         */
        cyng::db::session db_;

        /**
         * config/data cache
         */
        cfg &cfg_;
        cfg_http_post const cfg_http_post_;

        /**
         * parser for wireless M-Bus data
         */
        mbus::radio::parser parser_;
    };
} // namespace smf

#endif
