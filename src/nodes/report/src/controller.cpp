#include <controller.h>

#include <tasks/cluster.h>
#include <tasks/csv_report.h>
#include <tasks/lpex_report.h>

#include <smf.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>
#include <smf/report/config/cfg_feed_report.h>
#include <smf/report/config/cfg_lpex_report.h>
#include <smf/report/config/gen_csv.h>
#include <smf/report/config/gen_feed.h>
#include <smf/report/config/gen_gap.h>
#include <smf/report/config/gen_lpex.h>
#include <smf/report/csv.h>
#include <smf/report/feed.h>
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

            // gap reports
            cyng::make_param("gap", cfg::gen_gap(cwd)),

            // csv reports
            cyng::make_param("csv", cfg::gen_csv(cwd)),

            // LPEx reports
            cyng::make_param("lpex", cfg::gen_lpex(cwd)),

            // feed reports
            cyng::make_param("feed", cfg::gen_feed(cwd)),

            create_cluster_spec())});
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
            cyng::container_cast<cyng::param_map_t>(reader.get("feed")),
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
        cyng::param_map_t &&feed_reports,
        cyng::param_map_t &&gap_reports) {

        BOOST_ASSERT(!csv_reports.empty());

        cluster *tsk = nullptr;
        std::tie(cluster_, tsk) = ctl.create_named_channel_with_ref<cluster>(
            "report", ctl, channels, tag, node_name, logger, std::move(cfg_cluster), std::move(cfg_db));
        BOOST_ASSERT(cluster_->is_open());
        BOOST_ASSERT(tsk != nullptr);

        tsk->connect();

        cluster_->dispatch(
            "start.csv",
            //  handle dispatch errors
            std::bind(cyng::log_dispatch_error, logger, std::placeholders::_1, std::placeholders::_2),
            csv_reports);

        cluster_->dispatch(
            "start.lpex",
            //  handle dispatch errors
            std::bind(cyng::log_dispatch_error, logger, std::placeholders::_1, std::placeholders::_2),
            lpex_reports);

        cluster_->dispatch(
            "start.feed",
            //  handle dispatch errors
            std::bind(cyng::log_dispatch_error, logger, std::placeholders::_1, std::placeholders::_2),
            feed_reports);

        cluster_->dispatch(
            "start.egp",
            //  handle dispatch errors
            std::bind(cyng::log_dispatch_error, logger, std::placeholders::_1, std::placeholders::_2),
            gap_reports);
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
            // auto const cfg = read_config_section(config_.json_path_, config_.config_index_);

            if (boost::algorithm::equals(type, "csv")) {
                // cyng::container_cast<cyng::param_map_t>(reader.get("csv");
                generate_csv_reports(read_config_section(config_.json_path_, config_.config_index_));
            } else if (boost::algorithm::equals(type, "lpex")) {
                generate_lpex_reports(read_config_section(config_.json_path_, config_.config_index_));
            } else if (boost::algorithm::equals(type, "gap")) {
                generate_gap_reports(read_config_section(config_.json_path_, config_.config_index_));
            } else if (boost::algorithm::equals(type, "feed")) {
                generate_feed_reports(read_config_section(config_.json_path_, config_.config_index_));
            }
            return true; //  stop application
        }

        //
        //  dump readout data
        //
        if (!vars["dump"].defaulted()) {
            //	generate all reports
            auto const hours = std::chrono::hours(vars["dump"].as<int>() * 24);
            dump_readout(
                read_config_section(config_.json_path_, config_.config_index_),
                (hours < std::chrono::hours::zero()) ? (-1 * hours) : hours);
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
            auto const now = cyng::make_utc_date();
            auto reports = cyng::container_cast<cyng::param_map_t>(reader.get("csv"));

            for (auto const &cfg_report : reports) {
                auto const reader_report = cyng::make_reader(cfg_report.second);
                auto const name = reader_report.get("name", "no-name");

                if (reader_report.get("enabled", false)) {
                    auto const profile = cyng::to_obis(cfg_report.first);
                    std::cout << "***info: generate report " << name << " (" << profile << ")" << std::endl;
                    auto const root = reader_report.get("path", cwd.string());

                    if (!std::filesystem::exists(root)) {
                        std::cout << "***warning: output path [" << root << "] of report " << name << " does not exists"
                                  << std::endl;
                        std::error_code ec;
                        if (!std::filesystem::create_directories(root, ec)) {
                            std::cerr << "***error: cannot create path [" << root << "]: " << ec.message() << std::endl;
                        } else {
                            std::cout << "***info: path [" << root << "] created " << std::endl;
                        }
                    }

                    auto const prefix = reader_report.get("prefix", "");
                    generate_csv(s, profile, root, prefix, now, std::chrono::hours(40));
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

                //     "lpex": {
                //          "print.version": true,
                //          "debug": true,
                //          "filter": "0100010800ff:0100010801ff:0700030000ff",
                //          "profiles": {
                //              "8181c78611ff": { ... }
                //          }
                //      }

                cfg::lpex_report cfg(cyng::container_cast<cyng::param_map_t>(reader.get("lpex")));

                // auto const cwd = std::filesystem::current_path();
                auto const now = cyng::make_utc_date();
                using cyng::operator<<;
                std::cout << "***info: start LPEx reporting at " << now << " (UTC)" << std::endl;

                //  lpex/print.version
                auto const print_version = cfg.is_print_version();

                //  lpex/debug
                auto const debug_mode = cfg.is_debug_mode();

                //  lpex/filter
                auto const filter = cfg.get_filter();

                //  lpex/profiles/...
                auto const profiles = cfg.get_profiles();
                for (auto const &code : profiles) {
                    auto const name = cfg.get_name(code);

                    if (cfg.is_enabled(code)) {
                        std::cout << "***info: generate LPEx report " << name << " (" << code << ")" << std::endl;

                        auto const path = cfg.get_path(code);
                        if (sanitize_path(path)) {

                            generate_lpex(
                                s,
                                code,
                                filter,
                                path,
                                cfg.get_prefix(code),
                                now,
                                cfg.get_backtrack(code), //  backtrack hours
                                print_version,
                                debug_mode,
                                cfg.add_customer_data(code));
                        } else {
                            std::cerr << "***error: [" << path << "] of LPEx report " << name << " does not exist";
                        }

                    } else {
                        std::cout << "***info: report " << name << " is disabled" << std::endl;
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

                    auto const reader_report = cyng::make_reader(cfg_report.second);
                    auto const name = reader_report.get("name", "no-name");

                    if (reader_report.get("enabled", false)) {
                        auto const profile = cyng::to_obis(cfg_report.first);
                        std::cout << "***info: gap generate report " << name << " (" << profile << ")" << std::endl;
                        auto const root = reader_report.get("path", cwd.string());

                        if (!std::filesystem::exists(root)) {
                            std::cout << "***warning: output path [" << root << "] of report " << name << " does not exists"
                                      << std::endl;
                            std::error_code ec;
                            if (!std::filesystem::create_directories(root, ec)) {
                                std::cerr << "***error: cannot create path [" << root << "]: " << ec.message() << std::endl;
                            } else {
                                std::cout << "***info: path [" << root << "] created " << std::endl;
                            }
                        }
                        auto const backtrack = cyng::to_hours(reader_report.get("max.age", "40:00:00"));

                        generate_gap(s, profile, root, now, backtrack);

                    } else {
                        std::cout << "***info: gap report " << name << " is disabled" << std::endl;
                    }
                }

                return true;
            }
        } else {
            std::cerr << "***error: database [" << db_name << "] not found";
        }
        return false;
    }

    bool controller::generate_feed_reports(cyng::object &&cfg) {
        auto const reader = cyng::make_reader(std::move(cfg));

        auto const db_name = reader["DB"].get<std::string>("file.name", "");
        if (std::filesystem::exists(db_name)) {
            std::cout << "***info: file-name: " << db_name << std::endl;
            auto s = cyng::db::create_db_session(reader.get("DB"));
            if (s.is_alive()) {

                //     "feed": {
                //          "print.version": true,
                //          "debug": true,
                //          "filter": "0100010800ff:0100010801ff:0700030000ff",
                //          "profiles": {
                //              "8181c78611ff": { ... }
                //          }
                //      }

                auto const now = cyng::make_utc_date();
                using cyng::operator<<;
                std::cout << "***info: start reporting at " << now << " (UTC)" << std::endl;

                cfg::feed_report cfg(cyng::container_cast<cyng::param_map_t>(reader.get("feed")));

                auto const print_version = cfg.is_print_version();
                auto const debug_mode = cfg.is_debug_mode();

                //  feed/profiles/...
                auto const profiles = cfg.get_profiles();
                for (auto const &code : profiles) {
                    auto const name = cfg.get_name(code);

                    if (cfg.is_enabled(code)) {
                        std::cout << "***info: generate feed report " << name << " (" << code << ")" << std::endl;

                        auto const path = cfg.get_path(code);
                        if (sanitize_path(path)) {

                            generate_feed(
                                s,
                                code,
                                path,
                                cfg.get_prefix(code),
                                now,
                                cfg.get_backtrack(code), //  backtrack hours
                                print_version,
                                debug_mode,
                                cfg.add_customer_data(code));
                        } else {
                            std::cerr << "***error: [" << path << "] of feed report " << name << " does not exist";
                        }
                    }
                }
                return true;
            }
        }
        return false;
    }

    void controller::dump_readout(cyng::object &&cfg, std::chrono::hours backlog) {
        auto const now = cyng::make_utc_date();
        auto const reader = cyng::make_reader(cfg);
        // BOOST_ASSERT(reader.get("db").tag() == cyng::TC_PARAM_MAP);
        auto s = cyng::db::create_db_session(reader.get("DB"));
        if (s.is_alive()) {
            smf::dump_readout(s, now.get_end_of_day(), backlog);
        } else {
            std::cerr << "***error: database not found" << std::endl;
        }
    }

} // namespace smf
