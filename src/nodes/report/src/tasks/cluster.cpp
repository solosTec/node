/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#include <tasks/cluster.h>

#include <tasks/csv_report.h>
#include <tasks/gap_report.h>
#include <tasks/lpex_report.h>

#include <smf/obis/profile.h>

#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/parse/duration.h>
#include <cyng/parse/string.h>
#include <cyng/task/channel.h>

#include <iostream>

namespace smf {

    cluster::cluster(
        cyng::channel_weak wp,
        cyng::controller &ctl,
        cyng::stash& channels,
        boost::uuids::uuid tag,
        std::string const &node_name,
        cyng::logger logger,
        toggle::server_vec_t &&cfg,
        cyng::param_map_t &&cfg_db)
        : sigs_{
            std::bind(&cluster::connect, this), //  connect
            std::bind(&cluster::start_csv, this, std::placeholders::_1), //  start
            std::bind(&cluster::start_lpex, this, std::placeholders::_1), //  start
            std::bind(&cluster::start_gap, this, std::placeholders::_1), //  start
            std::bind(&cluster::stop, this, std::placeholders::_1) // stop
        }
        , channel_(wp)
        , ctl_(ctl)
        , channels_(channels)
        , tag_(tag)
        , logger_(logger)
        , fabric_(ctl)
        , bus_(ctl, logger, std::move(cfg), node_name, tag, NO_FEATURES, this)
        , db_(cyng::db::create_db_session(cfg_db)) {

        if (auto sp = channel_.lock(); sp) {
            sp->set_channel_names({"connect", "start.csv", "start.lpex", "start.gap"});
            CYNG_LOG_INFO(logger_, "task [" << sp->get_name() << "] started");
        }

        if (db_.is_alive()) {
            CYNG_LOG_INFO(logger_, "[cluster] database is open");
        } else {
            CYNG_LOG_FATAL(logger_, "[cluster] cannot open database: " << cfg_db);
        }
    }

    void cluster::stop(cyng::eod) {
        CYNG_LOG_WARNING(logger_, "stop cluster task(" << tag_ << ")");
        bus_.stop();
    }

    void cluster::connect() {
        //
        //	join cluster
        //
        bus_.start();
    }

    void cluster::start_csv(cyng::param_map_t reports) {

        //
        //	start reporting
        //
        for (auto const &cfg : reports) {
            auto const reader = cyng::make_reader(cfg.second);
            if (reader.get("enabled", false)) {

                auto const profile = cyng::to_obis(cfg.first);
                BOOST_ASSERT(sml::is_profile(profile));
                auto const name = reader.get("name", "");
                auto const path = reader.get("path", "");
                auto const backtrack = std::chrono::hours(reader.get("backtrack", sml::backtrack_time(profile).count()));
                auto const prefix = reader.get("prefix", "");

                auto channel =
                    ctl_.create_named_channel_with_ref<csv_report>(name, ctl_, logger_, db_, profile, path, backtrack, prefix)
                        .first;
                BOOST_ASSERT(channel->is_open());
                channels_.lock(channel);

                //
                //  calculate start time
                //
                auto const now = cyng::make_utc_date();
                auto const interval = sml::interval_time(now, profile);
                bus_.sys_msg(cyng::severity::LEVEL_INFO, "start csv report ", profile, " (", name, ") at ", interval, " UTC");

                channel->suspend(interval, "run");

            } else {
                CYNG_LOG_TRACE(logger_, "csv report " << cfg.first << " is disabled");
            }
        }
    }

    void cluster::start_lpex(cyng::param_map_t reports) {

        //
        //	start reporting
        //
        for (auto const &cfg : reports) {
            auto const reader = cyng::make_reader(cfg.second);
            if (reader.get("enabled", false)) {

                auto const profile = cyng::to_obis(cfg.first);
                BOOST_ASSERT(sml::is_profile(profile));
                auto const name = reader.get("name", "");
                auto const path = reader.get("path", "");
                //                auto const backtrack = std::chrono::hours(reader.get("backtrack",
                //                sml::backtrack_time(profile).count()));
                auto const prefix = reader.get("prefix", "");
                auto const backtrack = cyng::to_hours(reader.get("backtrack", "40:00:00"));
                auto const separated = reader.get("separated.by.devices", false);
                auto const customer = reader.get("add.customer.data", false);

                cyng::obis_path_t filter;
                auto channel = ctl_.create_named_channel_with_ref<lpex_report>(
                                       name,
                                       ctl_,
                                       logger_,
                                       db_,
                                       profile,
                                       filter, // filter
                                       path,
                                       backtrack,
                                       prefix,
                                       true,      // print version
                                       separated, // separated
                                       false,     // debug mode
                                       customer)
                                   .first;
                BOOST_ASSERT(channel->is_open());
                channels_.lock(channel);

                //
                //  calculate start time
                //
                auto const now = cyng::make_utc_date();
                auto const interval = sml::interval_time(now, profile);
                CYNG_LOG_INFO(logger_, "start lpex report " << profile << " (" << name << ") at " << interval);

                bus_.sys_msg(
                    cyng::severity::LEVEL_INFO, "start lpex report ", profile, " (", name, ") at ", now + interval, " UTC");
                channel->suspend(interval, "run");

            } else {
                CYNG_LOG_TRACE(logger_, "lpex report " << cfg.first << " is disabled");
            }
        }
    }

    void cluster::start_gap(cyng::param_map_t reports) {

        //
        //	start reporting
        //
        for (auto const &cfg : reports) {
            auto const reader = cyng::make_reader(cfg.second);
            if (reader.get("enabled", false)) {

                auto const profile = cyng::to_obis(cfg.first);
                BOOST_ASSERT(sml::is_profile(profile));
                auto const name = reader.get("name", "");
                auto const root = reader.get("path", "");
                auto const backtrack = cyng::to_hours(reader.get("max.age", "120:00:00"));
                // auto const prefix = reader.get("prefix", "");

                if (!std::filesystem::exists(root)) {
                    std::cout << "***warning: output path [" << root << "] of gap report " << name << " does not exists";
                    std::error_code ec;
                    if (!std::filesystem::create_directories(root, ec)) {
                        std::cerr << "***error: cannot create path [" << root << "]: " << ec.message();
                    }
                }

                auto channel =
                    ctl_.create_named_channel_with_ref<gap_report>(name, ctl_, logger_, db_, profile, root, backtrack).first;
                BOOST_ASSERT(channel->is_open());
                channels_.lock(channel);

                //
                //  calculate start time
                //
                auto const now = cyng::make_utc_date();
                auto const interval = sml::interval_time(now, profile);
                CYNG_LOG_INFO(logger_, "start gap report " << profile << " (" << name << ") at " << interval);

                bus_.sys_msg(cyng::severity::LEVEL_INFO, "start gap report ", profile, " (", name, ") at ", now + interval, " UTC");
                channel->suspend(interval, "run");

            } else {
                CYNG_LOG_TRACE(logger_, "gap report " << cfg.first << " is disabled");
            }
        }
    }

    //
    //	bus interface
    //
    cyng::mesh *cluster::get_fabric() { return &fabric_; }
    cfg_sink_interface *cluster::get_cfg_sink_interface() {
        //  not a data sink
        return nullptr;
    }
    cfg_data_interface *cluster::get_cfg_data_interface() { return nullptr; }

    void cluster::on_login(bool success) {
        if (success) {
            CYNG_LOG_INFO(logger_, "cluster join complete");

        } else {
            CYNG_LOG_ERROR(logger_, "joining cluster failed");
        }
    }
    void cluster::on_disconnect(std::string msg) { CYNG_LOG_WARNING(logger_, "[cluster] disconnect: " << msg); }

} // namespace smf
