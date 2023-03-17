#include <controller.h>

#include <smf.h>
#include <smf/imega.h>

#include <tasks/cluster.h>

#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/object.h>
#include <cyng/obj/util.hpp>
#include <cyng/parse/duration.h>
#include <cyng/task/registry.h>

#include <iostream>

namespace smf {

    controller::controller(config::startup const &config)
        : controller_base(config) {}

    void controller::run(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::object const &cfg,
        std::string const &node_name) {

#if _DEBUG_IMEGA
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
        auto const port = cyng::numeric_cast<std::uint16_t>(reader["server"]["port"].get(), 5200);
        auto const policy = imega::to_policy(cyng::value_cast(reader["server"]["policy"].get(), "global"));
        auto const pwd = cyng::value_cast(reader["server"]["global-pwd"].get(), "123456");

        auto const timeout_str = cyng::value_cast(reader["server"]["timeout"].get(), "00:00:12");
        auto const timeout = cyng::to_seconds(timeout_str);

        auto const wd_str = cyng::value_cast(reader["server"]["watchdog"].get(), "00:30:00");
        auto const wd = cyng::to_minutes(wd_str);

        //	connect to cluster
        //
        join_cluster(ctl, logger, tag, node_name, std::move(tgl), address, port, policy, pwd, timeout, wd);
    }
    void controller::shutdown(cyng::registry &reg, cyng::stash &channels, cyng::logger logger) {

        //
        //	stop all running tasks
        //
        reg.shutdown();
    }

    cyng::vector_t controller::create_default_config(
        std::chrono::system_clock::time_point &&now,
        std::filesystem::path &&tmp,
        std::filesystem::path &&cwd) {

        return cyng::make_vector({cyng::make_tuple(
            cyng::make_param("generated", now),
            cyng::make_param("version", SMF_VERSION_TAG),
            cyng::make_param("tag", get_random_tag()),
            create_server_spec(),
            create_cluster_spec())});
    }

    cyng::param_t controller::create_server_spec() {
        return cyng::make_param(
            "server",
            cyng::make_tuple(
                cyng::make_param("address", "0.0.0.0"),
                cyng::make_param("port", 5200),
                cyng::make_param("timeout", "00:00:12"),   // connection timeout
                cyng::make_param("watchdog", "00:30:00"),  // minutes
                cyng::make_param("pwd-policy", "global"),  // swibi/MNAME, sgsw/TELNB
                cyng::make_param("global-pwd", "12345678") // 8 characters
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
        imega::policy policy,
        std::string const &pwd,
        std::chrono::seconds timeout,
        std::chrono::minutes watchdog) {

        cluster *tsk = nullptr;
        std::tie(cluster_, tsk) = ctl.create_named_channel_with_ref<cluster>(
            "cluster", ctl, tag, node_name, logger, std::move(cfg), policy, pwd, timeout, watchdog);
        BOOST_ASSERT(cluster_->is_open());
        BOOST_ASSERT(tsk != nullptr);
        tsk->connect();

        auto const ep = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port);
        CYNG_LOG_INFO(logger, "server is listening at " << ep);
        //  handle dispatch errors
        cluster_->dispatch("listen", std::bind(cyng::log_dispatch_error, logger, std::placeholders::_1, std::placeholders::_2), ep);
    }

} // namespace smf
