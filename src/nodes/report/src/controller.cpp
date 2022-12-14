/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>

#include <tasks/cluster.h>
#include <tasks/csv_report.h>
#include <tasks/lpex_report.h>

#include <smf.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>
#include <smf/report/csv.h>
#include <smf/report/gap.h>
#include <smf/report/lpex.h>
#include <smf/report/utility.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/date.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/object.h>
#include <cyng/obj/util.hpp>
#include <cyng/parse/duration.h>
#include <cyng/parse/string.h>
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
            cyng::make_param("tag", get_random_tag()),
            cyng::make_param("country.code", cyng::sys::get_system_locale().at(cyng::sys::info::COUNTRY)),
            cyng::make_param("language.code", cyng::sys::get_system_locale().at(cyng::sys::info::LANGUAGE)),
            // cyng::make_param("utc.offset", cyng::sys::delta_utc(now).count()),

            cyng::make_param(
                "DB",
                cyng::make_tuple(
                    cyng::make_param("connection.type", "SQLite"),
                    cyng::make_param("file.name", (cwd / "store.database").string()),
                    cyng::make_param("busy.timeout", std::chrono::milliseconds(12)), // seconds
                    cyng::make_param("watchdog", std::chrono::seconds(30)),          // for database connection
                    cyng::make_param("pool.size", 1),                                // no pooling for SQLite
                    cyng::make_param("readonly", true)                               // no write access required
                    )),

            cyng::make_param(
                "csv",
                cyng::make_tuple(
                    create_csv_spec(OBIS_PROFILE_1_MINUTE, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MINUTE)),
                    create_csv_spec(OBIS_PROFILE_15_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_15_MINUTE)),
                    create_csv_spec(OBIS_PROFILE_60_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_60_MINUTE)),
                    create_csv_spec(OBIS_PROFILE_24_HOUR, cwd, true, sml::backtrack_time(OBIS_PROFILE_24_HOUR)),
                    // create_csv_spec(OBIS_PROFILE_LAST_2_HOURS, cwd, false, sml::backtrack_time(OBIS_PROFILE_LAST_2_HOURS)),
                    // create_csv_spec(OBIS_PROFILE_LAST_WEEK, cwd, false, sml::backtrack_time(OBIS_PROFILE_LAST_WEEK)),
                    create_csv_spec(OBIS_PROFILE_1_MONTH, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MONTH)), // one month
                    create_csv_spec(OBIS_PROFILE_1_YEAR, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_YEAR))    //  one year
                    )                                                                                             // csv reports
                ),

            cyng::make_param(
                "lpex",
                cyng::make_tuple(
                    cyng::make_param("print.version", true), // if true first line contains the LPEx version
                    cyng::make_param(
                        "debug",
#ifdef _DEBUG
                        true
#else
                        false
#endif
                        ), // if true the generated LPex files contain debug data
                    // 1-0:1.8.0*255    (01 00 01 08 00 FF)
                    // 1-0:1.8.1*255    (01 00 01 08 01 FF)
                    // 1-0:1.8.2*255    (01 00 01 08 02 FF)
                    // 1-0:16.7.0*255   (01 00 10 07 00 FF)
                    // 1-0:2.0.0*255    (01 00 02 00 00 FF)
                    // 7-0:3.0.0*255    (07 00 03 00 00 FF) // Volume (meter), temperature converted (Vtc), forward, absolute,
                    // current value 7-0:3.1.0*1      (07 00 03 01 00 01)
                    cyng::make_param(
                        "filter",
                        cyng::obis_path_t{OBIS_REG_POS_ACT_E, OBIS_REG_POS_ACT_E_T1, OBIS_REG_GAS_MC_0_0}), // only this obis codes
                                                                                                            // will be accepted
                    create_lpex_spec(OBIS_PROFILE_15_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_15_MINUTE)),
                    create_lpex_spec(OBIS_PROFILE_60_MINUTE, cwd, false, sml::backtrack_time(OBIS_PROFILE_60_MINUTE)),
                    create_lpex_spec(OBIS_PROFILE_24_HOUR, cwd, false, sml::backtrack_time(OBIS_PROFILE_24_HOUR)),
                    create_lpex_spec(OBIS_PROFILE_1_MONTH, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MONTH)) // one month
                    )                                                                                             // LPEx reports
                ),

            cyng::make_param(
                "gap",
                cyng::make_tuple(
                    create_gap_spec(OBIS_PROFILE_1_MINUTE, cwd, std::chrono::hours(48), false),
                    create_gap_spec(OBIS_PROFILE_15_MINUTE, cwd, std::chrono::hours(48), true),
                    create_gap_spec(OBIS_PROFILE_60_MINUTE, cwd, std::chrono::hours(800), true),
                    create_gap_spec(OBIS_PROFILE_24_HOUR, cwd, std::chrono::hours(800), true)) // gap reports
                ),

            create_cluster_spec())});
    }

    cyng::prop_t create_csv_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack) {
        return cyng::make_prop(
            profile,
            cyng::make_tuple(
                cyng::make_param("name", obis::get_name(profile)),
                cyng::make_param("path", (cwd / "csv.reports" / get_prefix(profile)).string()),
                cyng::make_param("backtrack", backtrack),
                cyng::make_param("prefix", ""),
                cyng::make_param("enabled", enabled)));
    }

    cyng::prop_t create_lpex_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack) {
        return cyng::make_prop(
            profile,
            cyng::make_tuple(
                cyng::make_param("name", obis::get_name(profile)),
                cyng::make_param("path", (cwd / "lpex-reports" / get_prefix(profile)).string()),
                cyng::make_param("backtrack", backtrack),
                cyng::make_param("prefix", "LPEx-"),
                cyng::make_param("separated.by.devices", false), // individual reports for each device
                cyng::make_param("enabled", enabled)));
    }

    cyng::prop_t create_gap_spec(cyng::obis profile, std::filesystem::path const &cwd, std::chrono::hours hours, bool enabled) {
        return cyng::make_prop(
            profile,
            cyng::make_tuple(
                cyng::make_param("name", obis::get_name(profile)),
                cyng::make_param("path", (cwd / "gap-reports" / get_prefix(profile)).string()),
                cyng::make_param("max.age", hours),
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
            cyng::container_cast<cyng::param_map_t>(reader.get("csv")),
            cyng::container_cast<cyng::param_map_t>(reader.get("lpex")),
            cyng::container_cast<cyng::param_map_t>(reader.get("gap")));
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
        cyng::param_map_t &&csv_reports,
        cyng::param_map_t &&lpex_reports,
        cyng::param_map_t &&gap_reports) {

        BOOST_ASSERT(!csv_reports.empty());

        cluster_ = ctl.create_named_channel_with_ref<cluster>(
            "report", ctl, channels, tag, node_name, logger, std::move(cfg_cluster), std::move(cfg_db));
        BOOST_ASSERT(cluster_->is_open());
        cluster_->dispatch("connect");
        cluster_->dispatch("start.csv", csv_reports);
        cluster_->dispatch("start.lpex", lpex_reports);
        cluster_->dispatch("start.egp", gap_reports);
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
        //  generate different reports
        //
        if (!vars["generate"].defaulted()) {
            //	generate all reports
            auto const type = vars["generate"].as<std::string>();
            if (boost::algorithm::equals(type, "csv")) {
                generate_csv_reports(read_config_section(config_.json_path_, config_.config_index_));
            } else if (boost::algorithm::equals(type, "lpex")) {
                generate_lpex_reports(read_config_section(config_.json_path_, config_.config_index_));
            } else if (boost::algorithm::equals(type, "gap")) {
                generate_gap_reports(read_config_section(config_.json_path_, config_.config_index_));
            }
            return true; //  stop application
        }

        //
        //	call base classe
        //
        return controller_base::run_options(vars);
    }

    bool controller::generate_csv_reports(cyng::object &&cfg) {

        auto const reader = cyng::make_reader(std::move(cfg));
        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            std::cout << "***info: file-name: " << reader["DB"].get<std::string>("file.name", "") << std::endl;
            auto const cwd = std::filesystem::current_path();
            auto const now = std::chrono::system_clock::now();
            auto reports = cyng::container_cast<cyng::param_map_t>(reader.get("csv"));

            for (auto const &cfg_report : reports) {
                auto const reader_report = cyng::make_reader(cfg_report.second);
                auto const name = reader_report.get("name", "no-name");

                if (reader_report.get("enabled", false)) {
                    auto const profile = cyng::to_obis(cfg_report.first);
                    std::cout << "***info: generate report " << name << " (" << profile << ")" << std::endl;
                    auto const root = reader_report.get("path", cwd.string());

                    if (!std::filesystem::exists(root)) {
                        std::cout << "***warning: output path [" << root << "] of report " << name << " does not exists";
                        std::error_code ec;
                        if (!std::filesystem::create_directories(root, ec)) {
                            std::cerr << "***error: cannot create path [" << root << "]: " << ec.message();
                        }
                    }

                    auto const prefix = reader_report.get("prefix", "");
                    generate_csv(s, profile, root, std::chrono::hours(40), now, prefix);
                    return true;

                } else {
                    std::cout << "***info: report " << name << " is disabled" << std::endl;
                }
            }
        }
        return false;
    }

    bool controller::generate_lpex_reports(cyng::object &&cfg) {

        auto const reader = cyng::make_reader(std::move(cfg));

        auto const db_name = reader["DB"].get<std::string>("file.name", "");
        if (std::filesystem::exists(db_name)) {
            std::cout << "***info: file-name: " << db_name << std::endl;
            auto s = cyng::db::create_db_session(reader.get("DB"));
            if (s.is_alive()) {
                //"print.version"
                // std::cout << "***info: file-name: " << reader["DB"].get<std::string>("file.name", "") << std::endl;
                auto const cwd = std::filesystem::current_path();
                auto const now = cyng::make_utc_date();
                using cyng::operator<<;
                std::cout << "***info: start reporting at " << now << " (UTC)" << std::endl;
                auto reports = cyng::container_cast<cyng::param_map_t>(reader.get("lpex"));

                auto const print_version = reader["lpex"].get("print.version", true);
                auto const debug_mode = reader["lpex"].get(
                    "debug",
#ifdef _DEBUG
                    true
#else
                    false
#endif
                );
                auto const filter = cyng::to_obis_path(reader["lpex"].get("filter", ""));
                for (auto const &cfg_report : reports) {
                    //
                    //  skip "print.version" entry "separated.by.devices"
                    //
                    if (!boost::algorithm::equals(cfg_report.first, "print.version") &&
                        !boost::algorithm::equals(cfg_report.first, "debug") &&
                        !boost::algorithm::equals(cfg_report.first, "filter") &&
                        !boost::algorithm::equals(cfg_report.first, "filter.obsolete")) {
                        auto const reader_report = cyng::make_reader(cfg_report.second);
                        auto const name = reader_report.get("name", "no-name");

                        if (reader_report.get("enabled", false)) {
                            auto const profile = cyng::to_obis(cfg_report.first);
                            std::cout << "***info: generate report " << name << " (" << profile << ")" << std::endl;
                            auto const root = reader_report.get("path", cwd.string());

                            if (!std::filesystem::exists(root)) {
                                std::cout << "***warning: output path [" << root << "] of report " << name << " does not exists";
                                std::error_code ec;
                                if (!std::filesystem::create_directories(root, ec)) {
                                    std::cerr << "***error: cannot create path [" << root << "]: " << ec.message();
                                }
                            }

                            auto const backtrack = cyng::to_hours(reader_report.get("backtrack", "40:00:00"));
                            auto const separated = reader_report.get("separated.by.devices", false);

                            auto const prefix = reader_report.get("prefix", "");
                            generate_lpex(
                                s,
                                profile,
                                filter,
                                root,
                                now,
                                backtrack, //  backtrack hours
                                prefix,
                                print_version,
                                separated,
                                debug_mode);

                        } else {
                            std::cout << "***info: report " << name << " is disabled" << std::endl;
                        }
                    }
                }

                return true;
            }
        } else {
            std::cerr << "***error: database [" << db_name << "] not found";
        }
        return false;
    }

    bool controller::generate_gap_reports(cyng::object &&cfg) {
        auto const reader = cyng::make_reader(std::move(cfg));

        auto const db_name = reader["DB"].get<std::string>("file.name", "");
        if (std::filesystem::exists(db_name)) {
            std::cout << "***info: file-name: " << db_name << std::endl;
            auto s = cyng::db::create_db_session(reader.get("DB"));
            if (s.is_alive()) {
                auto const cwd = std::filesystem::current_path();
                auto const now = cyng::make_utc_date();
                auto const reports = cyng::container_cast<cyng::param_map_t>(reader.get("gap"));

                for (auto const &cfg_report : reports) {
                    //
                    //  skip "print.version" and "filter" entry
                    //
                    if (!boost::algorithm::equals(cfg_report.first, "print.version") &&
                        !boost::algorithm::starts_with(cfg_report.first, "filter") &&
                        !boost::algorithm::starts_with(cfg_report.first, "debug")) {
                        auto const reader_report = cyng::make_reader(cfg_report.second);
                        auto const name = reader_report.get("name", "no-name");

                        if (reader_report.get("enabled", false)) {
                            auto const profile = cyng::to_obis(cfg_report.first);
                            std::cout << "***info: gap generate report " << name << " (" << profile << ")" << std::endl;
                            auto const root = reader_report.get("path", cwd.string());

                            if (!std::filesystem::exists(root)) {
                                std::cout << "***warning: output path [" << root << "] of report " << name << " does not exists";
                                std::error_code ec;
                                if (!std::filesystem::create_directories(root, ec)) {
                                    std::cerr << "***error: cannot create path [" << root << "]: " << ec.message();
                                }
                            }
                            auto const backtrack = cyng::to_hours(reader_report.get("max.age", "40:00:00"));

                            generate_gap(s, profile, root, now, backtrack);

                        } else {
                            std::cout << "***info: gap report " << name << " is disabled" << std::endl;
                        }
                    }
                }

                return true;
            }
        } else {
            std::cerr << "***error: database [" << db_name << "] not found";
        }
        return false;
    }

} // namespace smf
