/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>
#include <tasks/cluster.h>
#include <tasks/storage_db.h>

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

#include <iostream>
#include <locale>

#include <boost/algorithm/string.hpp>

namespace smf {

    controller::controller(config::startup const &config) : controller_base(config) {}

    cyng::vector_t controller::create_default_config(
        std::chrono::system_clock::time_point &&now, std::filesystem::path &&tmp, std::filesystem::path &&cwd) {

        std::locale loc(std::locale(), new std::ctype<char>);
        std::cout << std::locale("").name().c_str() << std::endl;

        return cyng::make_vector({cyng::make_tuple(
            cyng::make_param("generated", now), cyng::make_param("log-dir", tmp.string()),
            cyng::make_param("tag", get_random_tag()), cyng::make_param("country-code", "CH"),
            cyng::make_param("language-code", cyng::sys::get_system_locale()),

            cyng::make_param("storage", "DB"), //	options are XML, JSON, DB

            cyng::make_param(
                "DB",
                cyng::make_tuple(
                    cyng::make_param("connection-type", "SQLite"), cyng::make_param("file-name", (cwd / "setup.database").string()),
                    cyng::make_param("busy-timeout", 12), //	seconds
                    cyng::make_param("watchdog", 30),     //	for database connection
                    cyng::make_param("pool-size", 1)      //	no pooling for SQLite
                    )),

            cyng::make_param(
                "XML",
                cyng::make_tuple(
                    cyng::make_param("file-name", (cwd / "setup.config.xml").string()), cyng::make_param("endcoding", "UTF-8"))),

            cyng::make_param("JSON", cyng::tuple_factory(cyng::make_param("file-name", (cwd / "setup.config.json").string()))),

            create_cluster_spec())});
    }

    void controller::run(cyng::controller &ctl, cyng::logger logger, cyng::object const &cfg, std::string const &node_name) {
#if _DEBUG_SETUP
        CYNG_LOG_INFO(logger, cfg);
#endif
        auto const reader = cyng::make_reader(cfg);
        auto const tag = read_tag(reader["tag"].get());

        auto const storage_type = cyng::value_cast(reader["storage"].get(), "DB");
        CYNG_LOG_INFO(logger, "storage type is " << storage_type);

        auto tgl = read_config(cyng::container_cast<cyng::vector_t>(reader["cluster"].get()));
        BOOST_ASSERT(!tgl.empty());
        if (tgl.empty()) {
            CYNG_LOG_FATAL(logger, "no cluster data configured");
        }

        //
        //	connect to cluster
        //
        join_cluster(
            ctl, logger, tag, node_name, std::move(tgl), storage_type,
            cyng::container_cast<cyng::param_map_t>(reader[storage_type].get()));
    }

    void controller::shutdown(cyng::logger logger, cyng::registry &reg) {

        //
        //	stop all running tasks
        //
        reg.shutdown();
    }

    void controller::join_cluster(
        cyng::controller &ctl, cyng::logger logger, boost::uuids::uuid tag, std::string const &node_name,
        toggle::server_vec_t &&cfg_cluster, std::string storage_type, cyng::param_map_t &&cfg_db) {

        cluster_ = ctl.create_named_channel_with_ref<cluster>(
            "cluster", ctl, tag, node_name, logger, std::move(cfg_cluster), storage_type, std::move(cfg_db));
        BOOST_ASSERT(cluster_->is_open());
        cluster_->dispatch("connect", cyng::make_tuple());
    }

    cyng::param_t controller::create_cluster_spec() {
        return cyng::make_param(
            "cluster",
            cyng::make_vector({
                cyng::make_tuple(
                    cyng::make_param("host", "127.0.0.1"), cyng::make_param("service", "7701"), cyng::make_param("account", "root"),
                    cyng::make_param("pwd", "NODE_PWD"), cyng::make_param("salt", "NODE_SALT"),
                    cyng::make_param("group", 0)) //	customer ID
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

        auto const user = vars["user"].as<std::string>();
        if (!user.empty()) {
            //	generate a set of default access rights
            generate_access_rights(read_config_section(config_.json_path_, config_.config_index_), user);
            return true;
        }

        auto const dev_count = vars["gen-devices"].as<std::uint32_t>();
        if (dev_count != 0) {
            //	generate a random set of device configuration
            generate_random_devs(read_config_section(config_.json_path_, config_.config_index_), dev_count);
            return true;
        }

        //
        //	call base classe
        //
        return controller_base::run_options(vars);
    }

    void controller::generate_access_rights(cyng::object &&cfg, std::string const &user) {
        auto const reader = cyng::make_reader(std::move(cfg));
        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive())
            smf::generate_access_rights(s, user); //	see task/storage_db.h
    }
    void controller::init_storage(cyng::object &&cfg) {

        auto const reader = cyng::make_reader(std::move(cfg));
        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive())
            smf::init_storage(s); //	see task/storage_db.h
    }
    void controller::generate_random_devs(cyng::object &&cfg, std::uint32_t count) {

        auto const reader = cyng::make_reader(std::move(cfg));
        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive())
            smf::generate_random_devs(s, count); //	see task/storage_db.h
    }

} // namespace smf
