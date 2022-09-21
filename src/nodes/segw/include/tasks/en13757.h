/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_EN13757_H
#define SMF_SEGW_EN13757_H

#include <cfg.h>
#include <config/cfg_cache.h>
#include <config/cfg_sml.h>

#include <smf/mbus/radio/parser.h>

#include <cyng/log/logger.h>
#include <cyng/net/client_factory.hpp>
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
            std::function<void(void)>,        // reset_target_channels
            std::function<void(std::string)>, // add_target_channel
            std::function<void()>,            // push
            std::function<void(cyng::eod)>    // stop()
            >;

      public:
        en13757(cyng::channel_weak, cyng::controller &ctl, cyng::logger, cfg &);

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
         * Check if an AES key is available and if that is the case, decode the data.
         */
        void decode(mbus::radio::header const &h, mbus::radio::tplayer const &t, cyng::buffer_t const &data);
        void decode(
            cyng::table *tbl_readout,
            cyng::table *tbl_data,
            cyng::table *tbl_collector,
            cyng::table *tbl_mirror,
            srv_id_t address,
            std::uint8_t access_no,
            std::uint8_t frame_type,
            cyng::buffer_t const &data,
            cyng::crypto::aes_128_key const &,
            bool auto_cfg,
            cyng::obis def_profile);

        void read_sml(
            cyng::table *tbl_data,
            cyng::table *tbl_mirror,
            srv_id_t const &address,
            std::string const &id,
            cyng::buffer_t const &payload,
            bool auto_cfg,
            cyng::obis def_profile);
        void read_mbus(
            cyng::table *tbl_data,
            cyng::table *tbl_mirror,
            srv_id_t const &address,
            std::string const &id,
            std::uint8_t medium,
            std::string const &manufacturer,
            std::uint8_t frame_type,
            cyng::buffer_t const &payload,
            bool auto_cfg,
            cyng::obis def_profile);

        void update_cache(mbus::radio::header const &h, mbus::radio::tplayer const &tpl, cyng::buffer_t const &data);
        void update_load_profile(mbus::radio::header const &h, mbus::radio::tplayer const &tpl, cyng::buffer_t const &data);

        /**
         * send data and clear cache
         */
        void push();
        void push_data(cyng::channel_ptr);

      private:
        signatures_t sigs_;
        cyng::channel_weak channel_;

        cyng::controller &ctl_;
        cyng::net::client_factory client_factory_;

        /**
         * global logger
         */
        cyng::logger logger_;

        /**
         * config/data cache
         */
        cfg &cfg_;
        cfg_sml const cfg_sml_;
        cfg_cache const cfg_cache_;

        /**
         * parser for wireless M-Bus data
         */
        mbus::radio::parser parser_;
    };
} // namespace smf

#endif
