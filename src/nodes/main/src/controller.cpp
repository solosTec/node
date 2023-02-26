/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>

#include <session.h>
#include <smf.h>

#include <cyng/log/record.h>
#include <cyng/net/server_factory.hpp>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/object.h>
#include <cyng/obj/util.hpp>
#include <cyng/sys/locale.h>
#include <cyng/task/controller.h>

#include <iostream>
#include <locale>

namespace smf {

    controller::controller(config::startup const &config)
        : controller_base(config)
        , server_proxy_()
        , session_counter_{0u} {}

    cyng::vector_t controller::create_default_config(
        std::chrono::system_clock::time_point &&now,
        std::filesystem::path &&tmp,
        std::filesystem::path &&cwd) {

        return cyng::make_vector({cyng::make_tuple(
            cyng::make_param("generated", now),
            cyng::make_param("version", SMF_VERSION_TAG),
            cyng::make_param("tag", get_random_tag()),
            cyng::make_param("country.code", cyng::sys::get_system_locale().at(cyng::sys::info::COUNTRY)),
            cyng::make_param("language.code", cyng::sys::get_system_locale().at(cyng::sys::info::LANGUAGE)),
            create_server_spec(),
            create_session_spec(tmp),
            create_cluster_spec())});
    }

    cyng::param_t controller::create_cluster_spec() {
        return cyng::make_param(
            "cluster",
            cyng::make_tuple(
                cyng::make_param("account", "root"),
                cyng::make_param("pwd", "NODE_PWD"),
                cyng::make_param("salt", "NODE_SALT"),
                cyng::make_param("monitor", 58) //	seconds
                ));
    }

    cyng::param_t controller::create_server_spec() {
        return cyng::make_param(
            "server", cyng::make_tuple(cyng::make_param("address", "0.0.0.0"), cyng::make_param("service", "7701")));
    }
    cyng::param_t controller::create_session_spec(std::filesystem::path const &tmp) {
        //  this will passed to table "config"
        return cyng::make_param(
            "session",
            cyng::make_tuple(
                cyng::make_param("auto.login", false),
                cyng::make_param("auto.enabled", true),
                cyng::make_param("superseede", true),
                cyng::make_param("gw.cache", true),       //  no longer used
                cyng::make_param("auto.insert.gw", true), //  not active used yet
                cyng::make_param("generate-time-series", false),
                cyng::make_param("catch.meters", false),
                cyng::make_param("catch.lora", true),
                cyng::make_param("stat.dir", tmp.string()),                    //	store statistics
                cyng::make_param("max.messages", 1000),                        //	system log
                cyng::make_param("max.events", 2000),                          //	time series events
                cyng::make_param("max.LoRa.records", 500),                     //	LoRa uplink records
                cyng::make_param("max.wMBus.records", 500),                    //	wireless M-Bus uplink records
                cyng::make_param("max.IEC.records", 600),                      //	IECs uplink records
                cyng::make_param("max.bridges", 300),                          //	max. entries in TBridge
                cyng::make_param("def.IEC.interval", std::chrono::minutes(20)) //  IEC default readout interval in minutes
                ));
    }

    void controller::run(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::object const &cfg,
        std::string const &node_name) {

#if _DEBUG_MAIN
        CYNG_LOG_INFO(logger, cfg);
#endif
        auto const reader = cyng::make_reader(cfg);
        auto const tag = read_tag(reader["tag"].get());

        auto const country_code =
            cyng::value_cast(reader["country.code"].get(), cyng::sys::get_system_locale().at(cyng::sys::info::COUNTRY));
        auto const lang_code = cyng::value_cast(reader["language.code"].get(), "en-GB");

        auto const address = cyng::value_cast(reader["server"]["address"].get(), "0.0.0.0");
        auto const port = cyng::numeric_cast<std::uint16_t>(reader["server"]["port"].get(), 7701);
        auto const ep = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port);

        auto const account = cyng::value_cast(reader["cluster"]["account"].get(), "root");
        auto const pwd = cyng::value_cast(reader["cluster"]["pwd"].get(), "");
        auto const salt = cyng::value_cast(reader["cluster"]["salt"].get(), "");
        auto const monitor = cyng::numeric_cast<std::uint32_t>(reader["cluster"]["monitor"].get(), 58); //	seconds

        auto const session_cfg = customize_session_config(cyng::container_cast<cyng::param_map_t>(reader["session"].get()));

        /**
         *  data cache
         */
        static cyng::store store;
        static db cache(store, logger, tag);
        static cyng::mesh fabric(ctl);

        //
        //	create all tables and set initial values
        //
        cache.init(session_cfg, country_code, lang_code);

        cyng::net::server_factory srvf(ctl);
        server_proxy_ = srvf.create_proxy<boost::asio::ip::tcp::socket, 2048>(
            [this, &logger](boost::system::error_code ec) {
                if (!ec) {
                    CYNG_LOG_INFO(logger, "listen callback " << ec.message());
                } else {
                    CYNG_LOG_WARNING(logger, "listen callback " << ec.message());
                }
            },
            [this, &logger](boost::asio::ip::tcp::socket socket) {
                auto const ep = socket.remote_endpoint();
                CYNG_LOG_INFO(logger, "new session #" << session_counter_ << ": " << ep);
                auto sp =
                    std::shared_ptr<session>(new session(std::move(socket), cache, fabric, logger), [this, &logger](session *s) {
                        BOOST_ASSERT(s != nullptr);

                        //
                        //	remove from cluster table
                        //
                        auto const ptys = cache.remove_pty_by_peer(s->get_peer(), s->get_remote_peer());
                        CYNG_LOG_TRACE(logger, "session [" << s->get_peer() << "] with " << ptys << " users closed");

                        //
                        //	stop session and
                        //	remove all subscriptions
                        //
                        s->stop();

                        //
                        //	update session counter
                        //
                        BOOST_ASSERT(session_counter_ != 0);
                        --session_counter_;
                        CYNG_LOG_TRACE(logger, "session(s) running: " << session_counter_);
                        cache.sys_msg(cyng::severity::LEVEL_TRACE, "node is going offline - ", ptys, "#", session_counter_);

                        //
                        //	remove session
                        //
                        delete s;
                    });

                if (sp) {

                    //
                    //	start session
                    //
                    sp->start();

                    //
                    //	update session counter
                    //
                    ++session_counter_;
                    cache.sys_msg(cyng::severity::LEVEL_TRACE, "node comes online + ", ep, "#", session_counter_);
                }
            });

        //
        //  start cluster server
        //
        server_proxy_.listen(ep);
    }

    void controller::shutdown(cyng::registry &reg, cyng::stash &channels, cyng::logger logger) {

        server_proxy_.stop();
        config::stop_tasks(logger, reg, "main");

        //
        //	stop all running tasks
        //
        reg.shutdown();
    }

    cyng::param_map_t customize_session_config(cyng::param_map_t &&cfg) {
        for (auto &param : cfg) {
            param.second = tidy_config(param.first, param.second);
        }
        return cfg;
    }

} // namespace smf
