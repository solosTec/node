/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */

#include <controller.h>

#include <tasks/cluster.h>
#include <tasks/storage_db.h>

#include <smf.h>

#include <cyng/db/session.h>
#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/container.h>
// #include <cyng/obj/numeric_cast.hpp>
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

        return cyng::make_vector({cyng::make_tuple(
            cyng::make_param("generated", now),
            cyng::make_param("version", SMF_VERSION_TAG),
            cyng::make_param("tag", get_random_tag()),
            cyng::make_param("country.code", cyng::sys::get_system_locale().at(cyng::sys::info::COUNTRY)),
            cyng::make_param("language.code", cyng::sys::get_system_locale().at(cyng::sys::info::LANGUAGE)),
            create_db_spec(cwd),
            create_cluster_spec())});
    }

    cyng::param_t controller::create_db_spec(std::filesystem::path cwd) {
        return cyng::make_param(
            "DB",
            cyng::make_tuple(
                cyng::make_param("connection.type", "SQLite"),
                cyng::make_param("file.name", (cwd / "pull.database").string()),
                cyng::make_param("busy.timeout", std::chrono::milliseconds(12)), //	seconds
                cyng::make_param("watchdog", std::chrono::seconds(30)),          //	for database connection
                cyng::make_param("pool.size", 1)                                 //	no pooling for SQLite
                ));
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

    bool controller::run_options(boost::program_options::variables_map &vars) {

        //
        //
        //
        if (vars["init"].as<bool>()) {
            //	initialize database
            init_storage(read_config_section(config_.json_path_, config_.config_index_));
            return true;
        }

        //
        //	call base classe
        //
        return controller_base::run_options(vars);
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

        auto tgl = read_config(cyng::container_cast<cyng::vector_t>(reader["cluster"].get()));
        BOOST_ASSERT(!tgl.empty());
        if (tgl.empty()) {
            CYNG_LOG_FATAL(logger, "no cluster data configured");
        } else {

            //
            //	connect to cluster
            //
            join_cluster(
                ctl,
                logger,
                tag,
                node_name,
                std::move(tgl),                                             // cluster configuration
                cyng::container_cast<cyng::param_map_t>(reader["DB"].get()) // database configuration
            );
        }
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
        std::string const &node_name,
        toggle::server_vec_t &&cfg_cluster,
        cyng::param_map_t &&cfg_db) {

        cluster *tsk = nullptr;
        std::tie(cluster_, tsk) = ctl.create_named_channel_with_ref<cluster>(
            "cluster", ctl, tag, node_name, logger, std::move(cfg_cluster), std::move(cfg_db));
        BOOST_ASSERT(cluster_->is_open());
        BOOST_ASSERT(tsk != nullptr);
        if (tsk) {
            tsk->connect();
        } else {
            CYNG_LOG_FATAL(logger, "cannot create cluster channel");
        }
    }

    void controller::init_storage(cyng::object &&cfg) {

        auto const reader = cyng::make_reader(std::move(cfg));
        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            std::cout << "file-name: " << reader["DB"].get<std::string>("file.name", "") << std::endl;
            smf::init_storage(s); //	see storage_db.h
        }
    }

} // namespace smf