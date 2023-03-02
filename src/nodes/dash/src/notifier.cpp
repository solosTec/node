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

#include <boost/uuid/nil_generator.hpp>
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
                cyng::record rec(tbl->meta(), key, data, gen);
                update_meter_online_state(rec, event_type::INSERT);
                update_gw_online_state(rec, event_type::INSERT);
                update_meter_iec_online_state(rec, event_type::INSERT);
                update_meter_wmbus_online_state(rec, event_type::INSERT);
            }
        }

        return true;
    }

    bool notifier::forward(
        cyng::table const *tbl,
        cyng::key_t const &key,
        cyng::attr_t const &attr,
        cyng::data_t const &data,
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

            if (boost::algorithm::equals(r.table_, "session")) {
                cyng::record rec(tbl->meta(), key, data, gen);
                update_meter_online_state(rec, event_type::UPDATE);
                update_gw_online_state(rec, event_type::UPDATE);
                update_meter_iec_online_state(rec, event_type::UPDATE);
                update_meter_wmbus_online_state(rec, event_type::UPDATE);
            } else if (boost::algorithm::equals(r.table_, "device")) {
                cyng::record rec(tbl->meta(), key, data, gen);
                update_statistics_enabled_state(rec, event_type::UPDATE);
            }
        }
        return true;
    }

    bool notifier::forward(cyng::table const *tbl, cyng::key_t const &key, cyng::data_t const &data, boost::uuids::uuid tag) {
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
                cyng::record rec(tbl->meta(), key, data, 0);
                update_meter_online_state(rec, event_type::REMOVE);
                update_gw_online_state(rec, event_type::REMOVE);
                update_meter_iec_online_state(rec, event_type::REMOVE);
                update_meter_wmbus_online_state(rec, event_type::REMOVE);
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

    void notifier::update_meter_online_state(cyng::record const &rec, event_type evt) {

        auto const range = db_.subscriptions_.equal_range("config.meter");
        auto const count = std::distance(range.first, range.second);
        //	get column name
        auto const col_name = "online";
        CYNG_LOG_INFO(
            logger_,
            "[channel] "
                << "config.meter"
                << " update (#" << count << "): " << col_name << " => " << get_name(evt));

        auto const param = cyng::make_param(col_name, calc_connection_state(evt, rec));
        for (auto pos = range.first; pos != range.second; ++pos) {
            http_server_.notify_update("config.meter", rec.key(), param, pos->second);
        }
    }
    void notifier::update_gw_online_state(cyng::record const &rec, event_type evt) {
        auto const range = db_.subscriptions_.equal_range("config.gateway");
        auto const count = std::distance(range.first, range.second);
        //	get column name
        std::string const col_name = "online";
        CYNG_LOG_INFO(
            logger_,
            "[channel] "
                << "config.gateway"
                << " update (#" << count << "): " << col_name << " => " << get_name(evt));
        auto const param = cyng::make_param(col_name, calc_connection_state(evt, rec));
        for (auto pos = range.first; pos != range.second; ++pos) {
            http_server_.notify_update("config.gateway", rec.key(), param, pos->second);
        }
    }

    void notifier::update_meter_iec_online_state(cyng::record const &, event_type evt) {}
    void notifier::update_meter_wmbus_online_state(cyng::record const &, event_type evt) {}

    void notifier::update_statistics_enabled_state(cyng::record const &rec, event_type evt) {
        auto const range = db_.subscriptions_.equal_range("monitor.statistics");
        auto const count = std::distance(range.first, range.second);
        //	get column name
        std::string const col_name = "enabled";
        CYNG_LOG_INFO(
            logger_,
            "[channel] "
                << "monitor.statistics"
                << " update (#" << count << "): " << col_name << " => " << get_name(evt));
        //
        auto const tag = rec.value("tag", boost::uuids::nil_uuid());
        auto const enabled = rec.value("enabled", false);
        auto const param = cyng::make_param(col_name, enabled);
        for (auto pos = range.first; pos != range.second; ++pos) {
            http_server_.notify_update("monitor.statistics", rec.key(), param, pos->second);
        }
    }

    // static
    std::string notifier::get_name(event_type evt) {
        switch (evt) {
        case event_type::INSERT: return "INSERT";
        case event_type::UPDATE: return "UPDATE";
        case event_type::REMOVE: return "REMOVE";
        default: break;
        }
        return "UNKNOWN";
    }

    // static
    std::uint32_t notifier::calc_connection_state(event_type evt, cyng::record const &rec) {
        switch (evt) {
        case event_type::INSERT: return 1;
        case event_type::REMOVE: return 0;
        default: break;
        }

        auto const tag = rec.value("tag", boost::uuids::nil_uuid());
        auto const ctag = rec.value("cTag", boost::uuids::nil_uuid());

        if (ctag == boost::uuids::nil_uuid()) {
            //  not connected but online
            return 1;
        } else if (ctag == tag) {
            //  internal connection
            return 3;
        }
        //  somehow connected
        return 2;
    }

} // namespace smf
