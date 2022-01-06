/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */

#include <controller.h>
#include <smf.h>

#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/object.h>
#include <cyng/obj/util.hpp>
//#include <cyng/sys/locale.h>
#include <cyng/task/registry.h>

#include <iostream>
#include <locale>

namespace smf {

    controller::controller(config::startup const &config)
        : controller_base(config) {}

    cyng::vector_t controller::create_default_config(
        std::chrono::system_clock::time_point &&now,
        std::filesystem::path &&tmp,
        std::filesystem::path &&cwd) {

        return cyng::make_vector({cyng::make_tuple(
            cyng::make_param("generated", now),
            cyng::make_param("version", SMF_VERSION_TAG),
            cyng::make_param("log-dir", tmp.string()),
            cyng::make_param("tag", get_random_tag()),
            create_server_spec(cwd),
            create_cluster_spec())});
    }
    void controller::run(
        cyng::controller &,
        cyng::stash &channels,
        cyng::logger,
        cyng::object const &cfg,
        std::string const &node_name) {

        auto const reader = cyng::make_reader(cfg);
        auto const tag = read_tag(reader["tag"].get());
    }
    void controller::shutdown(cyng::registry &reg, cyng::stash &channels, cyng::logger logger) {

        //
        //	stop all running tasks
        //
        reg.shutdown();
    }

    cyng::param_t controller::create_server_spec(std::filesystem::path const &cwd) {
        return cyng::make_param(
            "server",
            cyng::make_tuple(
                cyng::make_param("address", "192.168.1.26"),
                cyng::make_param("service", "53"),
#if defined(BOOST_OS_LINUX_AVAILABLE)
                cyng::make_param("interface", "eth0"), //  wlan0
#else
                cyng::make_param("interface", "Ethernet"),
#endif
                cyng::make_param("gateway", "192.168.1.1"),
                cyng::make_param("provider", "1.1.1.1"), //	DNS provider (CloudFlare DNS)
                cyng::make_param("suffix", "test.local") //    DNS suffix
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

} // namespace smf
