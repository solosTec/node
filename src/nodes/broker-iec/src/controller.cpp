/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>
#include <smf.h>
#include <tasks/cluster.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/object.h>
#include <cyng/obj/util.hpp>
#include <cyng/obj/vector_cast.hpp>
#include <cyng/sys/locale.h>
#include <cyng/task/controller.h>
#include <cyng/task/stash.h>
#include <smf/ipt/scramble_key_format.h>

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
            // cyng::make_param("network.delay", 10), //  seconds to wait before starting ip-t client
            create_client_spec(),
            create_cluster_spec(),
            create_ipt_spec(tag),
            create_push_spec())});
    }

    void controller::run(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::object const &cfg,
        std::string const &node_name) {
#if _DEBUG_BROKER_IEC
        CYNG_LOG_INFO(logger, cfg);
#endif
        auto const reader = cyng::make_reader(cfg);
        auto const tag = read_tag(reader["tag"].get());

        auto tgl_cluster = read_config(cyng::container_cast<cyng::vector_t>(reader["cluster"].get()));
        BOOST_ASSERT(!tgl_cluster.empty());
        if (tgl_cluster.empty()) {
            CYNG_LOG_FATAL(logger, "no cluster data configured");
        }

        auto const client_login = cyng::value_cast(reader["client"]["login"].get(), false);
        // #if defined(BOOST_OS_WINDOWS_AVAILABLE)
        //         std::filesystem::path const client_out = cyng::value_cast(reader["client"]["out"].get(),
        //         "D:\\projects\\data\\csv");
        // #else
        //         std::filesystem::path const client_out = cyng::value_cast(reader["client"]["out"].get(), "/data/csv");
        // #endif
        //  if (!std::filesystem::exists(client_out)) {
        //      CYNG_LOG_FATAL(logger, "output path not found: [" << client_out << "]");
        //  }
        auto const reconnect_timeout = cyng::numeric_cast<std::size_t>(reader["client"]["reconnect.timeout"].get(), 40);
        auto const filter_enabled = cyng::numeric_cast<std::size_t>(reader["client"]["filter.enabled"].get(), false);
        // cyng::make_param("filter.enabled", false), //	use filter
        //    cyng::make_param("filter", cyng::make_vector({"12345678"}))
        auto const filter = (filter_enabled) ? cyng::vector_cast<std::string>(reader["client"]["filter"].get(), "00000000")
                                             : std::vector<std::string>();

        //
        //  ip-t configuration
        //
        auto const ipt_vec = cyng::container_cast<cyng::vector_t>(reader["ipt"].get());
        auto tgl_ipt = ipt::read_config(ipt_vec);
        if (tgl_ipt.empty()) {

            CYNG_LOG_FATAL(logger, "no ip-t server configured");

            auto const pwd = boost::uuids::to_string(tag);
            CYNG_LOG_INFO(logger, "use fallback configuration broker-wmbus:" << pwd << "@localhost:26862");

            tgl_ipt.push_back(ipt::server(
                "localhost",
                "26862",
                "broker-iec",
                pwd,
                ipt::to_sk("0102030405060708090001020304050607080900010203040506070809000001"),
                false,
                12));

            tgl_ipt.push_back(ipt::server(
                "ipt",
                "26862",
                "broker-iec",
                pwd,
                ipt::to_sk("0102030405060708090001020304050607080900010203040506070809000001"),
                false,
                12));
        }

        //
        //	connect to cluster
        //
        join_cluster(
            ctl,
            channels,
            logger,
            tag,
            node_name,
            std::move(tgl_cluster),
            client_login,
            (reconnect_timeout < 10) ? 10 : reconnect_timeout,
            std::move(tgl_ipt),
            // std::chrono::seconds(delay),
            ipt::read_push_channel_config(cyng::container_cast<cyng::param_map_t>(reader["push-channel"].get())));
    }

    void controller::shutdown(cyng::registry &reg, cyng::stash &channels, cyng::logger logger) {

        //
        //	stop all running tasks
        //
        channels.stop();
        channels.clear();
    }

    void controller::join_cluster(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        boost::uuids::uuid tag,
        std::string const &node_name,
        toggle::server_vec_t &&tgl_cluster,
        bool login,
        std::size_t reconnect_timeout,
        ipt::toggle::server_vec_t &&tgl_ipt,
        ipt::push_channel &&pcc) {

        auto channel = ctl.create_named_channel_with_ref<cluster>(
            "cluster",
            ctl,
            tag,
            node_name,
            logger,
            std::move(tgl_cluster),
            login,
            reconnect_timeout,
            std::move(tgl_ipt),
            std::move(pcc));

        BOOST_ASSERT(channel->is_open());
        channel->dispatch("connect");
        channels.lock(channel);
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
                cyng::make_param("verbose", false), //	parser
                                                    // #if defined(BOOST_OS_WINDOWS_AVAILABLE)
                //                cyng::make_param("out", "D:\\projects\\data\\csv"), //	output path
                // #else
                //                cyng::make_param("out", "/data/csv"), //	output path
                // #endif
                cyng::make_param("reconnect.timeout", 40), //	delay between reconnects in seconds
                cyng::make_param("filter.enabled", false), //	use filter
                cyng::make_param("filter", cyng::make_vector({"12345678"}))

                    ));
    }

    cyng::param_t controller::create_push_spec() {
        return cyng::make_param(
            "push-channel",
            cyng::make_tuple(
                cyng::make_param("target", "iec@store"),
                cyng::make_param("account", ""),
                cyng::make_param("number", ""),
                cyng::make_param("version", ""),
                cyng::make_param("id", ""),
                cyng::make_param("timeout", 30)));
    }

    cyng::param_t controller::create_ipt_spec(boost::uuids::uuid tag) {

        auto const pwd = boost::uuids::to_string(tag);

        return cyng::make_param(
            "ipt",
            cyng::make_vector(
                {cyng::make_tuple(
                     cyng::make_param("host", "localhost"),
                     cyng::make_param("service", "26862"),
                     cyng::make_param("account", "broker-iec"),
                     cyng::make_param("pwd", pwd),
                     cyng::make_param("def.sk", "0102030405060708090001020304050607080900010203040506070809000001"), //	scramble key
                     cyng::make_param("scrambled", true)),
                 cyng::make_tuple(
                     cyng::make_param("host", "ipt"),
                     cyng::make_param("service", "26862"),
                     cyng::make_param("account", "broker-iec"),
                     cyng::make_param("pwd", pwd),
                     cyng::make_param("def.sk", "0102030405060708090001020304050607080900010203040506070809000001"), //	scramble key
                     cyng::make_param("scrambled", false))}));
    }

} // namespace smf
