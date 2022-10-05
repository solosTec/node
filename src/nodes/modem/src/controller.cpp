/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>
#include <tasks/cluster.h>

#include <smf.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/object.h>
#include <cyng/obj/util.hpp>
// #include <cyng/sys/locale.h>
#include <cyng/task/controller.h>
#include <cyng/task/registry.h>

#include <iostream>
// #include <locale>

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
            cyng::make_param("tag", get_random_tag()),
            create_server_spec(),
            create_cluster_spec())});
    }
    void controller::run(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::object const &cfg,
        std::string const &node_name) {

#if _DEBUG_MODEM
        CYNG_LOG_INFO(logger, cfg);
#endif
        auto const reader = cyng::make_reader(cfg);
        auto const tag = read_tag(reader["tag"].get());

        auto tgl = read_config(cyng::container_cast<cyng::vector_t>(reader["cluster"].get()));
        BOOST_ASSERT(!tgl.empty());
        if (tgl.empty()) {
            CYNG_LOG_FATAL(logger, "no cluster data configured");
        }

        auto const address = cyng::value_cast(reader["server"]["address"].get(), "0.0.0.0");
        auto const port = cyng::numeric_cast<std::uint16_t>(reader["server"]["port"].get(), 9000);
        auto const answer = cyng::value_cast(reader["server"]["auto-answer"].get(), true);
#ifdef _DEBUG
        auto const guard = cyng::numeric_cast<std::uint64_t>(reader["server"]["guard-time"].get(), 30);
#else
        auto const guard = cyng::numeric_cast<std::uint64_t>(reader["server"]["guard-time"].get(), 10);
#endif
        auto const timeout = cyng::numeric_cast<std::uint64_t>(reader["server"]["timeout"].get(), 10);

        //
        //	connect to cluster
        //
        join_cluster(
            ctl,
            logger,
            tag,
            node_name,
            std::move(tgl),
            address,
            port,
            answer,
            std::chrono::milliseconds(guard),
            std::chrono::seconds(timeout));
    }

    void controller::shutdown(cyng::registry &reg, cyng::stash &channels, cyng::logger logger) {

        config::stop_tasks(logger, reg, "cluster");

        //
        //	stop all running tasks
        //
        reg.shutdown();
    }
    cyng::param_t controller::create_server_spec() {
        return cyng::make_param(
            "server",
            cyng::make_tuple(
                cyng::make_param("address", "0.0.0.0"),
                cyng::make_param("port", 9000),
                cyng::make_param("auto-answer", true), //	accept incoming calls automatically
#ifdef _DEBUG
                cyng::make_param("guard-time", 0), //	no guard time
#else
                cyng::make_param("guard-time", 1000), //	milliseconds
#endif
                cyng::make_param("timeout", 10) //	connection timeout in seconds (gatekeeper)
                ));
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
                    // cyng::make_param("monitor", rnd_monitor()),	//	seconds
                    cyng::make_param("group", 0)) //	customer ID
            }));
    }

    void controller::join_cluster(
        cyng::controller &ctl,
        cyng::logger logger,
        boost::uuids::uuid tag,
        std::string const &node_name,
        toggle::server_vec_t &&cfg,
        std::string const &address,
        std::uint16_t port,
        bool answer,
        std::chrono::milliseconds guard,
        std::chrono::seconds timeout) {

        cluster_ = ctl.create_named_channel_with_ref<cluster>(
            "cluster", ctl, tag, node_name, logger, std::move(cfg), answer, guard, timeout);
        BOOST_ASSERT(cluster_->is_open());
        cluster_->dispatch("connect");

        auto const ep = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port);
        cluster_->dispatch("listen", ep);
    }
} // namespace smf
