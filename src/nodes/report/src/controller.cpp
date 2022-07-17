/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>

#include <tasks/cluster.h>
#include <tasks/report.h>

#include <smf.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>
#include <smf/report/report.h>

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

#include <boost/algorithm/string.hpp>

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
            cyng::make_param("country-code", cyng::sys::get_system_locale().at(cyng::sys::info::COUNTRY)),
            cyng::make_param("language-code", cyng::sys::get_system_locale().at(cyng::sys::info::LANGUAGE)),

            cyng::make_param(
                "DB",
                cyng::make_tuple(
                    cyng::make_param("connection-type", "SQLite"),
                    cyng::make_param("file-name", (cwd / "store.sqlite").string()),
                    cyng::make_param("busy-timeout", 12), //	seconds
                    cyng::make_param("watchdog", 30),     //	for database connection
                    cyng::make_param("pool-size", 1)      //	no pooling for SQLite
                    )),

            cyng::make_param(
                "reports",
                cyng::make_tuple(
                    create_report_spec(OBIS_PROFILE_1_MINUTE, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MINUTE)),
                    create_report_spec(OBIS_PROFILE_15_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_15_MINUTE)),
                    create_report_spec(OBIS_PROFILE_60_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_60_MINUTE)),
                    create_report_spec(OBIS_PROFILE_24_HOUR, cwd, true, sml::backtrack_time(OBIS_PROFILE_24_HOUR)),
                    create_report_spec(OBIS_PROFILE_LAST_2_HOURS, cwd, false, sml::backtrack_time(OBIS_PROFILE_LAST_2_HOURS)),
                    create_report_spec(OBIS_PROFILE_LAST_WEEK, cwd, false, sml::backtrack_time(OBIS_PROFILE_LAST_WEEK)),
                    create_report_spec(OBIS_PROFILE_1_MONTH, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MONTH)), // one month
                    create_report_spec(OBIS_PROFILE_1_YEAR, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_YEAR))    //  one year
                    )                                                                                                // reports
                ),

            create_cluster_spec())});
    }

    cyng::prop_t create_report_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack) {
        return cyng::make_prop(
            profile,
            cyng::make_tuple(
                cyng::make_param("name", obis::get_name(profile)),
                cyng::make_param("path", (cwd / cyng::to_string(profile)).string()),
                cyng::make_param("backtrack", backtrack.count()),
                cyng::make_param("prefix", ""),
                cyng::make_param("enabled", enabled)));
    }

    void controller::run(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::object const &cfg,
        std::string const &node_name) {
#if _DEBUG_REPORT
        CYNG_LOG_INFO(logger, cfg);
#endif
        auto const reader = cyng::make_reader(cfg);
        auto const tag = read_tag(reader.get("tag"));

        auto tgl = read_config(cyng::container_cast<cyng::vector_t>(reader.get("cluster")));
        BOOST_ASSERT(!tgl.empty());
        if (tgl.empty()) {
            CYNG_LOG_FATAL(logger, "no cluster data configured");
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
            std::move(tgl),
            cyng::container_cast<cyng::param_map_t>(reader.get("DB")),
            cyng::container_cast<cyng::param_map_t>(reader.get("reports")));
    }

    void controller::shutdown(cyng::registry &reg, cyng::stash &channels, cyng::logger logger) {

        channels.stop();
        cluster_->stop();
        // config::stop_tasks(logger, reg, "report");

        //
        //	stop all running tasks
        //
        reg.shutdown();
    }

    void controller::join_cluster(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        boost::uuids::uuid tag,
        std::string const &node_name,
        toggle::server_vec_t &&cfg_cluster,
        cyng::param_map_t &&cfg_db,
        cyng::param_map_t &&cfg_reports) {

        BOOST_ASSERT(!cfg_reports.empty());

        cluster_ = ctl.create_named_channel_with_ref<cluster>(
            "report", ctl, channels, tag, node_name, logger, std::move(cfg_cluster), std::move(cfg_db));
        BOOST_ASSERT(cluster_->is_open());
        cluster_->dispatch("connect");
        cluster_->dispatch("start", cfg_reports);
    }

    cyng::param_t create_cluster_spec() {
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

    bool controller::run_options(boost::program_options::variables_map &vars) {

        //
        //
        //
        if (vars["generate"].as<bool>()) {
            //	generate all reports
            generate_reports(read_config_section(config_.json_path_, config_.config_index_));
            return true;
        }

        //
        //	call base classe
        //
        return controller_base::run_options(vars);
    }

    void controller::generate_reports(cyng::object &&cfg) {

        auto const reader = cyng::make_reader(std::move(cfg));
        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            std::cout << "***info: file-name: " << reader["DB"].get<std::string>("file-name", "") << std::endl;
            auto const cwd = std::filesystem::current_path();
            auto const now = std::chrono::system_clock::now();

            std::cout << "***info: generate 15 minute reports: " << std::endl;
            generate_report(s, OBIS_PROFILE_15_MINUTE, cwd, std::chrono::hours(40), now);

            std::cout << "***info: generate 60 minute reports: " << std::endl;
            generate_report(s, OBIS_PROFILE_60_MINUTE, cwd, std::chrono::hours(40), now);
        }
    }

} // namespace smf
