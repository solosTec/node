/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <db.h>
#include <http_server.h>
#include <notifier.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/container_factory.hpp>

#include <iostream>

namespace smf {

    notifier::notifier(db &data, http_server &srv, cyng::logger logger)
        : db_(data)
        , http_server_(srv)
        , logger_(logger) {}

    bool notifier::forward(
        cyng::table const *tbl,
        cyng::key_t const &key,
        cyng::data_t const &data,
        std::uint64_t gen,
        boost::uuids::uuid) {

        //	insert
        //
        //	notify all subscriptions
        //
        auto const r = db_.by_table(tbl->meta().get_name());
        if (!r.empty()) {

            {
                auto const range = db_.subscriptions_.equal_range(r.channel_);
                auto const count = std::distance(range.first, range.second);
                CYNG_LOG_TRACE(logger_, "update channel (insert): " << r.channel_ << " of " << count << " ws");
                for (auto pos = range.first; pos != range.second; ++pos) {
                    http_server_.notify_insert(r.channel_, cyng::record(tbl->meta(), key, data, gen), pos->second);
                }
            }
            {
                auto const range = db_.subscriptions_.equal_range(r.counter_);
                auto const count = std::distance(range.first, range.second);
                CYNG_LOG_TRACE(logger_, "update channel (size): " << r.counter_ << " of " << count << " ws");
                for (auto pos = range.first; pos != range.second; ++pos) {
                    http_server_.notify_size(r.counter_, tbl->size(), pos->second);
                }
            }

            if (boost::algorithm::equals(r.table_, "session")) {

                //
                //	update online state of
                //	meter, meterwMBus, meterIEC
                //
                update_meter_online_state(key, true);
                update_gw_online_state(key, true);
                update_meter_iec_online_state(key, true);
                update_meter_wmbus_online_state(key, true);
            }
        }

        return true;
    }

    bool notifier::forward(
        cyng::table const *tbl,
        cyng::key_t const &key,
        cyng::attr_t const &attr,
        std::uint64_t gen,
        boost::uuids::uuid tag) {

        //	update
        //
        //	notify all subscriptions
        //
        auto const r = db_.by_table(tbl->meta().get_name());
        if (!r.empty()) {

            auto const range = db_.subscriptions_.equal_range(r.channel_);
            auto const count = std::distance(range.first, range.second);
            //	get column name
            auto const col_name = tbl->meta().get_body_column(attr.first).name_;
            CYNG_LOG_INFO(
                logger_, "[channel] " << r.channel_ << " update (#" << count << "): " << col_name << " => " << attr.second);
            auto const param = cyng::param_t(col_name, attr.second);
            for (auto pos = range.first; pos != range.second; ++pos) {
                http_server_.notify_update(r.channel_, key, param, pos->second);
            }
        }
        return true;
    }

    bool notifier::forward(cyng::table const *tbl, cyng::key_t const &key, boost::uuids::uuid tag) {
        //	remove

        //
        //	notify all subscriptions
        //
        auto const r = db_.by_table(tbl->meta().get_name());
        if (!r.empty()) {

            if (boost::algorithm::equals(r.table_, "session")) {

                //
                //	update online state of
                //	meter, meterwMBus, meterIEC
                //
                update_meter_online_state(key, false);
                update_gw_online_state(key, false);
                update_meter_iec_online_state(key, false);
                update_meter_wmbus_online_state(key, false);
            }

            {
                auto range = db_.subscriptions_.equal_range(r.channel_);
                auto const count = std::distance(range.first, range.second);
                CYNG_LOG_TRACE(logger_, "update channel (delete): " << r.channel_ << " of " << count << " ws");
                for (auto pos = range.first; pos != range.second; ++pos) {
                    http_server_.notify_remove(r.channel_, key, pos->second);
                }
            }
            {
                auto const range = db_.subscriptions_.equal_range(r.counter_);
                auto const count = std::distance(range.first, range.second);
                CYNG_LOG_TRACE(logger_, "update channel (size): " << r.counter_ << " of " << count << " ws");
                for (auto pos = range.first; pos != range.second; ++pos) {
                    http_server_.notify_size(r.counter_, tbl->size(), pos->second);
                }
            }
        }

        return true;
    }

    bool notifier::forward(cyng::table const *tbl, boost::uuids::uuid) {
        //	clear

        //
        //	notify all subscriptions
        //
        auto const r = db_.by_table(tbl->meta().get_name());
        if (!r.empty()) {

            {
                auto const range = db_.subscriptions_.equal_range(r.channel_);
                auto const count = std::distance(range.first, range.second);
                CYNG_LOG_TRACE(logger_, "update channel (clear): " << r.channel_ << " of " << count << " ws");
                for (auto pos = range.first; pos != range.second; ++pos) {
                    http_server_.notify_clear(r.channel_, pos->second);
                }
            }
            {
                auto const range = db_.subscriptions_.equal_range(r.counter_);
                auto const count = std::distance(range.first, range.second);
                CYNG_LOG_TRACE(logger_, "update channel (size): " << r.counter_ << " of " << count << " ws");
                for (auto pos = range.first; pos != range.second; ++pos) {
                    http_server_.notify_size(r.counter_, 0, pos->second);
                }
            }
        }

        return true;
    }

    /**
     * transaction
     */
    bool notifier::forward(cyng::table const *, bool) { return true; }

    void notifier::update_meter_online_state(cyng::key_t const &key, bool online) {

        auto const range = db_.subscriptions_.equal_range("config.meter");
        auto const count = std::distance(range.first, range.second);
        //	get column name
        auto const col_name = "online";
        CYNG_LOG_INFO(
            logger_,
            "[channel] "
                << "config.meter"
                << " update (#" << count << "): " << col_name << " => " << (online ? "on" : "off"));
        auto const param = cyng::make_param(col_name, online ? 1 : 0);
        for (auto pos = range.first; pos != range.second; ++pos) {
            http_server_.notify_update("config.meter", key, param, pos->second);
        }
    }
    void notifier::update_gw_online_state(cyng::key_t const &key, bool online) {
        auto const range = db_.subscriptions_.equal_range("config.gateway");
        auto const count = std::distance(range.first, range.second);
        //	get column name
        std::string const col_name = "online";
        CYNG_LOG_INFO(
            logger_,
            "[channel] "
                << "config.gateway"
                << " update (#" << count << "): " << col_name << " => " << (online ? "on" : "off"));
        auto const param = cyng::make_param(col_name, online ? 1 : 0);
        for (auto pos = range.first; pos != range.second; ++pos) {
            http_server_.notify_update("config.gateway", key, param, pos->second);
        }
    }

    void notifier::update_meter_iec_online_state(cyng::key_t const &key, bool online) {}
    void notifier::update_meter_wmbus_online_state(cyng::key_t const &key, bool online) {}

} // namespace smf
