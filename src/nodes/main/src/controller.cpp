/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>
#include <tasks/server.h>

#include <smf.h>

#include <cyng/log/record.h>
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
        , cluster_() {}

    cyng::vector_t controller::create_default_config(
        std::chrono::system_clock::time_point &&now,
        std::filesystem::path &&tmp,
        std::filesystem::path &&cwd) {

        std::locale loc(std::locale(), new std::ctype<char>);
        std::cout << std::locale("").name().c_str() << std::endl;

        return cyng::make_vector({cyng::make_tuple(
            cyng::make_param("generated", now),
            cyng::make_param("version", SMF_VERSION_TAG),
            cyng::make_param("log-dir", tmp.string()),
            cyng::make_param("tag", get_random_tag()),
            cyng::make_param("country-code", "CH"),
            cyng::make_param("language-code", cyng::sys::get_system_locale()),
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
        return cyng::make_param(
            "session",
            cyng::make_tuple(
                cyng::make_param("auto-login", false),
                cyng::make_param("auto-enabled", true),
                cyng::make_param("superseede", true),
                cyng::make_param("gw-cache", true),
                cyng::make_param("generate-time-series", false),
                cyng::make_param("catch-meters", false),
                cyng::make_param("catch-lora", true),
                cyng::make_param("stat-dir", tmp.string()), //	store statistics
                cyng::make_param("max-messages", 1000),     //	system log
                cyng::make_param("max-events", 2000),       //	time series events
                cyng::make_param("max-LoRa-records", 500),  //	LoRa uplink records
                cyng::make_param("max-wMBus-records", 500), //	wireless M-Bus uplink records
                cyng::make_param("max-IEC-records", 600),   //	IECs uplink records
                cyng::make_param("max-bridges", 300)        //	max. entries in TBridge
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

        auto const country_code = cyng::value_cast(reader["country-code"].get(), "CH");
        auto const lang_code = cyng::value_cast(reader["language-code"].get(), "en-GB");

        auto const address = cyng::value_cast(reader["server"]["address"].get(), "0.0.0.0");
        auto const port = cyng::numeric_cast<std::uint16_t>(reader["server"]["port"].get(), 7701);
        auto const ep = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port);

        auto const account = cyng::value_cast(reader["cluster"]["account"].get(), "root");
        auto const pwd = cyng::value_cast(reader["cluster"]["pwd"].get(), "");
        auto const salt = cyng::value_cast(reader["cluster"]["salt"].get(), "");
        auto const monitor = cyng::numeric_cast<std::uint32_t>(reader["cluster"]["monitor"].get(), 58); //	seconds

        auto const session_cfg = cyng::container_cast<cyng::param_map_t>(reader["session"].get());

        cluster_ = ctl.create_named_channel_with_ref<server>(
            "main", ctl, tag, country_code, lang_code, logger, account, pwd, salt, std::chrono::seconds(monitor), session_cfg);
        BOOST_ASSERT(cluster_->is_open());

        cluster_->dispatch("start", cyng::make_tuple(ep));
    }

    void controller::shutdown(cyng::registry &reg, cyng::stash &channels, cyng::logger logger) {

        config::stop_tasks(logger, reg, "main");

        //
        //	stop all running tasks
        //
        reg.shutdown();
    }

} // namespace smf
