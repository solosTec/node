/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>
#include <tasks/cluster.h>
#include <tasks/push.h>

#include <smf.h>
#include <smf/ipt/scramble_key_format.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/object.h>
#include <cyng/obj/util.hpp>
#include <cyng/sys/locale.h>
#include <cyng/task/controller.h>
#include <cyng/task/stash.h>

#include <iostream>
#include <locale>

#include <boost/predef.h>

namespace smf {

    controller::controller(config::startup const &config)
        : controller_base(config) {}

    cyng::vector_t controller::create_default_config(
        std::chrono::system_clock::time_point &&now,
        std::filesystem::path &&tmp,
        std::filesystem::path &&cwd) {

        std::locale loc(std::locale(), new std::ctype<char>);
        std::cout << std::locale("").name().c_str() << std::endl;

        auto const tag = get_random_tag();

        return cyng::make_vector({cyng::make_tuple(
            cyng::make_param("generated", now),
            cyng::make_param("version", SMF_VERSION_TAG),
            cyng::make_param("tag", tag),
            cyng::make_param("country.code", cyng::sys::get_system_locale().at(cyng::sys::info::COUNTRY)),
            cyng::make_param("language.code", cyng::sys::get_system_locale().at(cyng::sys::info::LANGUAGE)),
            cyng::make_param("network.delay", 20), //  seconds to wait before starting ip-t client
            create_server_spec(cwd),
            create_push_spec(),
            create_client_spec(),
            create_cluster_spec(),
            create_ipt_spec(tag))});
    }

    void controller::run(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::object const &cfg,
        std::string const &node_name) {

#if _DEBUG_BROKER_WMBUS
        CYNG_LOG_INFO(logger, cfg);
#endif
        auto const reader = cyng::make_reader(cfg);
        auto const tag = read_tag(reader["tag"].get());

        auto tgl_cluster = read_config(cyng::container_cast<cyng::vector_t>(reader["cluster"].get()));
        BOOST_ASSERT(!tgl_cluster.empty());
        if (tgl_cluster.empty()) {
            CYNG_LOG_FATAL(logger, "no cluster data configured");
        }

        auto const address = cyng::value_cast(reader["server"]["address"].get(), "0.0.0.0");
        auto const port = cyng::numeric_cast<std::uint16_t>(reader["server"]["service"].get(), 12002);
        auto const client_login = cyng::value_cast(reader["client"]["login"].get(), false);
        std::chrono::seconds client_timeout(cyng::numeric_cast<std::uint32_t>(reader["client"]["timeout"].get(), 32));
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
        std::filesystem::path const client_out = cyng::value_cast(reader["client"]["out"].get(), "D:\\projects\\data\\csv");
#else
        std::filesystem::path const client_out = cyng::value_cast(reader["client"]["out"].get(), "/data/csv");
#endif

        //
        //	connect to cluster
        //
        join_cluster(
            ctl, channels, logger, tag, node_name, std::move(tgl_cluster), address, port, client_login, client_timeout, client_out);

        auto const ipt_vec = cyng::container_cast<cyng::vector_t>(reader["ipt"].get());
        auto tgl_ipt = ipt::read_config(ipt_vec);
        if (tgl_ipt.empty()) {

            CYNG_LOG_FATAL(logger, "no ip-t server configured");

            auto const pwd = boost::uuids::to_string(tag);
            CYNG_LOG_INFO(logger, "use fallback configuration broker-wmbus:" << pwd << "@localhost:26862");

            tgl_ipt.push_back(ipt::server(
                "localhost",
                "26862",
                "broker-wmbus",
                pwd,
                ipt::to_sk("0102030405060708090001020304050607080900010203040506070809000001"),
                false,
                12));
            tgl_ipt.push_back(ipt::server(
                "ipt",
                "26862",
                "broker-wmbus",
                pwd,
                ipt::to_sk("0102030405060708090001020304050607080900010203040506070809000001"),
                false,
                12));
        }

        //
        //  seconds to wait before starting ip-t client
        //
        auto const delay = cyng::numeric_cast<std::uint32_t>(reader["network.delay"].get(), 20);
        CYNG_LOG_INFO(logger, "start ipt bus in " << delay << " seconds");

        //
        //	connect to ip-t server
        //
        join_network(
            ctl,
            channels,
            logger,
            tag,
            node_name,
            std::move(tgl_ipt),
            std::chrono::seconds(delay),
            ipt::read_push_channel_config(cyng::container_cast<cyng::param_map_t>(reader["push-channel"]["SML"].get())),
            ipt::read_push_channel_config(cyng::container_cast<cyng::param_map_t>(reader["push-channel"]["IEC"].get())),
            ipt::read_push_channel_config(cyng::container_cast<cyng::param_map_t>(reader["push-channel"]["DLMS"].get())));
    }

    void controller::join_network(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        boost::uuids::uuid tag,
        std::string const &node_name,
        ipt::toggle::server_vec_t &&tgl,
        std::chrono::seconds delay,
        ipt::push_channel &&pc_sml,
        ipt::push_channel &&pcc_iec,
        ipt::push_channel &&pcc_dlms) {

        auto channel = ctl.create_named_channel_with_ref<push>(
            "push", ctl, logger, std::move(tgl), std::move(pc_sml), std::move(pcc_iec), std::move(pcc_dlms));
        BOOST_ASSERT(channel->is_open());

        //
        //  suspended to wait for ip-t node
        //
        channel->suspend(delay, "connect", cyng::make_tuple());
        channels.lock(channel);
    }

    void controller::shutdown(cyng::registry &reg, cyng::stash &channels, cyng::logger logger) {

        // stop "network" and "cluster"
        channels.stop();
        channels.clear();
    }

    void controller::join_cluster(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        boost::uuids::uuid tag,
        std::string const &node_name,
        toggle::server_vec_t &&tgl,
        std::string const &address,
        std::uint16_t port,
        bool client_login,
        std::chrono::seconds client_timeout,
        std::filesystem::path client_out) {

        auto channel = ctl.create_named_channel_with_ref<cluster>(
            "cluster", ctl, tag, node_name, logger, std::move(tgl), client_login, client_timeout, client_out);
        BOOST_ASSERT(channel->is_open());

        auto const ep = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port);
        channel->dispatch("connect");
        channel->suspend(std::chrono::seconds(20), "listen", ep);
        channels.lock(channel);
    }

    cyng::param_t controller::create_server_spec(std::filesystem::path const &cwd) {
        return cyng::make_param(
            "server", cyng::make_tuple(cyng::make_param("address", "0.0.0.0"), cyng::make_param("service", "12000")));
    }
    cyng::param_t controller::create_push_spec() {
        return cyng::make_param(
            "push-channel",
            cyng::make_tuple(
                cyng::make_param(
                    "SML",
                    cyng::param_map_factory("target", "sml@store") // ipt target
                    ("account", "")                                // unused
                    ("number", "")                                 // unused
                    ("version", "")                                // unused
                    ("timeout", 30)
                        .
                        operator cyng::param_map_t()),
                cyng::make_param(
                    "IEC",
                    cyng::param_map_factory("target", "iec@store") // ipt target
                    ("account", "")                                // unused
                    ("number", "")                                 // unused
                    ("version", "")                                // unused
                    ("timeout", 30)                                // unused
                        .
                        operator cyng::param_map_t()),
                cyng::make_param(
                    "DLMS",
                    cyng::param_map_factory("target", "dlms@store") // ipt target
                    ("account", "")                                 // unused
                    ("number", "")                                  // unused
                    ("version", "")                                 // unused
                    ("timeout", 30)
                        .
                        operator cyng::param_map_t())));
    }

    cyng::param_t controller::create_cluster_spec() {
        return cyng::make_param(
            "cluster",
            cyng::make_vector({
                cyng::make_tuple(
                    cyng::make_param("host", "127.0.0.1"),
                    cyng::make_param("service", "7701"),
                    cyng::make_param("account", "root"),
                    cyng::make_param("pwd", "NODE_PWD"),
                    cyng::make_param("salt", "NODE_SALT"),
                    cyng::make_param("group", 0)) //	customer ID
            }));
    }

    cyng::param_t controller::create_client_spec() {
        return cyng::make_param(
            "client",
            cyng::make_tuple(
                cyng::make_param("login", false),
                cyng::make_param("timeout", std::chrono::seconds(32)), //	gatekeeper
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
                cyng::make_param("out", "D:\\projects\\data\\csv") //	output path
#else
                cyng::make_param("out", "/data/csv") //	output path
#endif

                ));
    }

    cyng::param_t controller::create_ipt_spec(boost::uuids::uuid tag) {

        auto const pwd = boost::uuids::to_string(tag);

        return cyng::make_param(
            "ipt",
            cyng::make_vector(
                {cyng::make_tuple(
                     cyng::make_param("host", "localhost"),
                     cyng::make_param("service", "26862"),
                     cyng::make_param("account", "broker-wmbus"),
                     cyng::make_param("pwd", pwd),
                     cyng::make_param("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"), //	scramble key
                     cyng::make_param("scrambled", true)),
                 cyng::make_tuple(
                     cyng::make_param("host", "ipt"),
                     cyng::make_param("service", "26862"),
                     cyng::make_param("account", "broker-wmbus"),
                     cyng::make_param("pwd", pwd),
                     cyng::make_param("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"), //	scramble key
                     cyng::make_param("scrambled", false))}));
    }

} // namespace smf
