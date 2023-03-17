/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>
#include <tasks/cluster.h>

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
#include <cyng/parse/mac.h>
#include <cyng/sys/locale.h>
#include <cyng/sys/mac.h>
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
            //            cyng::make_param("log.dir", tmp.string()),
            cyng::make_param("tag", get_random_tag()),
            cyng::make_param("country.code", cyng::sys::get_system_locale().at(cyng::sys::info::COUNTRY)),
            cyng::make_param("language.code", cyng::sys::get_system_locale().at(cyng::sys::info::LANGUAGE)),
            cyng::make_param("query", 6),
            create_server_spec(),
            create_client_spec(),
            create_cluster_spec())});
    }

    cyng::param_t controller::create_server_spec() {
        return cyng::make_param(
            "server",
            cyng::make_tuple(
                cyng::make_param("address", "0.0.0.0"),
                cyng::make_param("port", 26862),
                cyng::make_param("sk", "0102030405060708090001020304050607080900010203040506070809000001"), //	scramble key
                cyng::make_param("watchdog", 30), //	for IP-T connection (minutes)
                cyng::make_param("timeout", 10)   //	connection timeout in seconds
                ));
    }

    cyng::param_t controller::create_client_spec() {
        //	get adapter data
        //
        auto macs = cyng::sys::get_mac48_adresses();
        if (macs.empty()) {
            //	random private MAC
            macs.push_back(cyng::generate_random_mac48());
        }

        return cyng::make_param("client", cyng::make_tuple(cyng::make_param("mac", macs.front())));
    }

    cyng::param_t controller::create_cluster_spec() {
        return cyng::make_param(
            "cluster",
            cyng::make_vector({
                //	redundancy I
                cyng::make_tuple(
                    cyng::make_param("host", "localhost"),
                    cyng::make_param("service", "7701"),
                    cyng::make_param("account", "root"),
                    cyng::make_param("pwd", "NODE_PWD"),
                    cyng::make_param("salt", 756)) //	ToDo: use project wide defined value
            }));
    }

    void controller::run(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::object const &cfg,
        std::string const &node_name) {

#if _DEBUG_IPT
        CYNG_LOG_INFO(logger, cfg);
#endif
        auto const reader = cyng::make_reader(cfg);
        auto const tag = read_tag(reader["tag"].get());
        auto const query = cyng::numeric_cast<std::uint32_t>(reader["query"].get(), 6u);

        auto tgl = read_config(cyng::container_cast<cyng::vector_t>(reader["cluster"].get()));
        BOOST_ASSERT(!tgl.empty());
        if (tgl.empty()) {
            CYNG_LOG_FATAL(logger, "no cluster data configured");
        }

        auto const address = cyng::value_cast(reader["server"]["address"].get(), "0.0.0.0");
        auto const port = cyng::numeric_cast<std::uint16_t>(reader["server"]["port"].get(), 26862);
        auto const sk = ipt::to_sk(
            cyng::value_cast(reader["server"]["sk"].get(), "0102030405060708090001020304050607080900010203040506070809000001"));
        auto const watchdog = cyng::numeric_cast<std::uint16_t>(reader["server"]["watchdog"].get(), 30);
        auto const timeout = cyng::numeric_cast<std::uint16_t>(reader["server"]["timeout"].get(), 10);

        //  client mac
        auto def_mac = cyng::to_string(cyng::generate_random_mac48());
        auto const client_mac = cyng::to_mac48(cyng::value_cast(reader["client"]["mac"].get(), def_mac));

        //
        //	connect to cluster
        //
        join_cluster(
            ctl,
            logger,
            tag,
            query,
            node_name,
            std::move(tgl),
            address,
            port,
            sk,
            std::chrono::minutes(watchdog),
            std::chrono::seconds(timeout),
            client_mac);
    }

    void controller::shutdown(cyng::registry &reg, cyng::stash &channels, cyng::logger logger) {

        config::stop_tasks(logger, reg, "cluster");

        //
        //	stop all running tasks
        //
        reg.shutdown();
    }

    void controller::join_cluster(
        cyng::controller &ctl,
        cyng::logger logger,
        boost::uuids::uuid tag,
        std::uint32_t query,
        std::string const &node_name,
        toggle::server_vec_t &&tgl,
        std::string const &address,
        std::uint16_t port,
        ipt::scramble_key const &sk,
        std::chrono::minutes watchdog,
        std::chrono::seconds timeout,
        cyng::mac48 client_id) {

        cluster *tsk = nullptr;
        std::tie(cluster_, tsk) = ctl.create_named_channel_with_ref<cluster>(
            "cluster", ctl, tag, query, node_name, logger, std::move(tgl), sk, watchdog, timeout, client_id);
        BOOST_ASSERT(cluster_->is_open());
        BOOST_ASSERT(tsk != nullptr);
        tsk->connect();

        auto const ep = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port);
        //  handle dispatch errors
        cluster_->dispatch("listen", std::bind(cyng::log_dispatch_error, logger, std::placeholders::_1, std::placeholders::_2), ep);
    }

} // namespace smf
