#include <controller.h>

#include <tasks/cluster.h>

#include <smf.h>

#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/object.h>
#include <cyng/obj/util.hpp>
#include <cyng/task/registry.h>

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

        return cyng::make_vector({cyng::make_tuple(
            cyng::make_param("generated", now),
            cyng::make_param("version", SMF_VERSION_TAG),
            cyng::make_param("tag", get_random_tag()),
            create_server_spec(cwd),
            create_cluster_spec())});
    }
    void controller::run(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::object const &cfg,
        std::string const &node_name) {

        auto const reader = cyng::make_reader(cfg);
        auto const tag = read_tag(reader["tag"].get());

        auto tgl = read_config(cyng::container_cast<cyng::vector_t>(reader["cluster"].get()));
        BOOST_ASSERT(!tgl.empty());
        if (tgl.empty()) {
            CYNG_LOG_FATAL(logger, "no cluster data configured");
        }

        //
        //	connect to cluster
        //
        join_cluster(
            ctl,
            logger,
            tag,
            node_name,
            std::move(tgl),
            cyng::value_cast(reader["server"]["address"].get(), "0.0.0.0"),
            cyng::numeric_cast<std::uint16_t>(reader["server"]["port"].get(), 53));
    }

    void controller::join_cluster(
        cyng::controller &ctl,
        cyng::logger logger,
        boost::uuids::uuid tag,
        std::string const &node_name,
        toggle::server_vec_t &&cfg,
        std::string &&address,
        std::uint16_t port) {

        // auto const ep = boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(address), port);
        auto const ep = boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port);
        CYNG_LOG_INFO(logger, "server is listening at " << ep);

        cluster *tsk = nullptr;
        std::tie(cluster_, tsk) =
            ctl.create_named_channel_with_ref<cluster>("cluster", ctl, tag, node_name, logger, std::move(cfg), ep);
        BOOST_ASSERT(cluster_->is_open());
        BOOST_ASSERT(tsk != nullptr);
        tsk->connect();
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
                cyng::make_param("port", 53),        // plain
                cyng::make_param("port_crypt", 443), // DNSCrypt
                cyng::make_param("port_doh", 443),   // DNS-over-HTTPS
                cyng::make_param("port_dot", 853),   // DNS-over-TLS
                cyng::make_param("port_doq", 853),   // DNS-over-QUIC
#if defined(BOOST_OS_LINUX_AVAILABLE)
                cyng::make_param("interface", "eth0"), //  wlan0
#else
                cyng::make_param("interface", "Ethernet"),
#endif
                cyng::make_param("gateway", "192.168.1.1"),
                cyng::make_param("provider", "1.1.1.1"), //	DNS provider (CloudFlare DNS)
                cyng::make_param("suffix", "test.local") // DNS suffix
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
