#include <controller.h>

#include <config/generator.h>
#include <influxdb.h>

#include <tasks/cleanup_db.h>
#include <tasks/csv_report.h>
#include <tasks/feed_report.h>
#include <tasks/gap_report.h>
#include <tasks/lpex_report.h>
#include <tasks/network.h>

#include <tasks/dlms_influx_writer.h>
#include <tasks/iec_csv_writer.h>
#include <tasks/iec_db_writer.h>
#include <tasks/iec_influx_writer.h>
#include <tasks/iec_json_writer.h>
#include <tasks/iec_log_writer.h>
#include <tasks/sml_abl_writer.h>
#include <tasks/sml_csv_writer.h>
#include <tasks/sml_db_writer.h>
#include <tasks/sml_influx_writer.h>
#include <tasks/sml_json_writer.h>
#include <tasks/sml_log_writer.h>
#include <tasks/sml_xml_writer.h>

#include <smf/obis/conv.h>
#include <smf/obis/profile.h>
#include <smf/report/config/cfg_cleanup.h>
#include <smf/report/config/cfg_csv_report.h>
#include <smf/report/config/cfg_feed_report.h>
#include <smf/report/config/cfg_gap.h>
#include <smf/report/config/cfg_lpex_report.h>
#include <smf/report/csv.h>
#include <smf/report/feed.h>
#include <smf/report/gap.h>
#include <smf/report/lpex.h>
#include <smf/report/utility.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp> //  ToDo: remove
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/date.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/object.h>
#include <cyng/obj/set_cast.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/obj/vector_cast.hpp>
#include <cyng/parse/duration.h> // ToDo: remove
#include <cyng/parse/string.h>
#include <cyng/sys/locale.h>
#include <cyng/task/controller.h>

#include <iostream>
#include <locale>

#include <boost/algorithm/string.hpp>

namespace smf {

    controller::controller(config::startup const &config)
        : controller_base(config) {}

    cyng::vector_t controller::create_default_config(
        std::chrono::system_clock::time_point &&now,
        std::filesystem::path &&tmp,
        std::filesystem::path &&cwd) {

        //
        //  <config/generator.h>
        //
        return smf::create_default_config(std::move(now), std::move(tmp), std::move(cwd), get_random_tag());
    }

    void controller::run(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::object const &cfg,
        std::string const &node_name) {

        auto const reader = cyng::make_reader(cfg);
        auto const tag = read_tag(reader["tag"].get());
        auto const now = std::chrono::system_clock::now();
        auto const model = cyng::value_cast(reader["model"].get(), "smf.store");

        auto const ipt_vec = cyng::container_cast<cyng::vector_t>(reader["ipt"].get());
        auto tgl = ipt::read_config(ipt_vec);
        if (tgl.empty()) {
            CYNG_LOG_FATAL(logger, "no ip-t server configured");
        }

        //
        //  data base connections
        //
        auto sm = init_storage(cfg, false);

        //
        //  start cleanup tasks
        //
        for (auto &s : sm) {
            if (s.second.is_alive()) {
                start_cleanup_tasks(
                    ctl, logger, s.first, s.second, cyng::container_cast<cyng::param_map_t>(reader["db"][s.first].get("cleanup")));
            }
        }

        //
        //  start gap report tasks
        //
        for (auto &s : sm) {
            if (s.second.is_alive()) {
                start_gap_reports(
                    ctl, logger, s.first, s.second, cyng::container_cast<cyng::param_map_t>(reader["db"][s.first].get("gap")));
            }
        }

        //
        // Start writer tasks.
        // No duplicates possible by using a set
        //
        auto const writer = cyng::set_cast<std::string>(reader["writer"].get(), "ALL:BIN");
        if (!writer.empty()) {
            auto const tmp = std::filesystem::temp_directory_path();
            for (auto const &name : writer) {
                if (boost::algorithm::equals(name, "SML:DB")) {
                    auto const db = reader[name].get("db", "default");
                    auto const pos = sm.find(db);
                    if (pos != sm.end()) {
                        auto const vec = reader[name].get("exclude.profiles", cyng::vector_t{});
                        auto const excludes = obis::to_obis_set(vec);
                        start_sml_db(ctl, channels, logger, pos->second, name, excludes);
                    } else {
                        CYNG_LOG_FATAL(logger, "no database [" << db << "] for writer " << name << " configured");
                    }
                } else if (boost::algorithm::equals(name, "SML:XML")) {
                    start_sml_xml(
                        ctl,
                        channels,
                        logger,
                        name,
                        cyng::value_cast(reader[name]["root-dir"].get(), tmp.string()),
                        cyng::value_cast(reader[name]["prefix"].get(), "sml"),
                        cyng::value_cast(reader[name]["suffix"].get(), "csv"));
                } else if (boost::algorithm::equals(name, "SML:JSON")) {
                    start_sml_json(
                        ctl,
                        channels,
                        logger,
                        name,
                        cyng::value_cast(reader[name]["root-dir"].get(), tmp.string()),
                        cyng::value_cast(reader[name]["prefix"].get(), "sml"),
                        cyng::value_cast(reader[name]["suffix"].get(), "csv"));
                } else if (boost::algorithm::equals(name, "SML:ABL")) {

#ifdef _MSC_VER
                    auto const line_ending = cyng::value_cast(reader[name]["line-ending"].get(), "DOS");
#else
                    auto const line_ending = cyng::value_cast(reader[name]["line-ending"].get(), "UNIX");
#endif

                    start_sml_abl(
                        ctl,
                        channels,
                        logger,
                        name,
                        cyng::value_cast(reader[name]["root-dir"].get(), tmp.string()),
                        cyng::value_cast(reader[name]["prefix"].get(), "sml"),
                        cyng::value_cast(reader[name]["suffix"].get(), "csv"),
                        boost::algorithm::equals(line_ending, "DOS"));

                } else if (boost::algorithm::equals(name, "SML:LOG")) {
                    start_sml_log(
                        ctl,
                        channels,
                        logger,
                        name,
                        cyng::value_cast(reader[name]["root-dir"].get(), tmp.string()),
                        cyng::value_cast(reader[name]["prefix"].get(), "sml"),
                        cyng::value_cast(reader[name]["suffix"].get(), "csv"),
                        cyng::value_cast(reader[name]["header"].get(), true));
                } else if (boost::algorithm::equals(name, "SML:CSV")) {
                    start_sml_csv(
                        ctl,
                        channels,
                        logger,
                        name,
                        cyng::value_cast(reader[name]["root-dir"].get(), tmp.string()),
                        cyng::value_cast(reader[name]["prefix"].get(), "sml"),
                        cyng::value_cast(reader[name]["suffix"].get(), "csv"),
                        cyng::value_cast(reader[name]["header"].get(), true));
                } else if (boost::algorithm::equals(name, "SML:influxdb")) {

                    start_sml_influx(
                        ctl,
                        channels,
                        logger,
                        name,
                        cyng::value_cast(reader[name]["host"].get(), "localhost"),
                        cyng::value_cast(reader[name]["port"].get(), "8086"),
                        cyng::value_cast(reader[name]["protocol"].get(), "http"),
                        cyng::value_cast(reader[name]["cert"].get(), "cert.pem"),
                        cyng::value_cast(reader[name]["db"].get(), "SMF"));
                } else if (boost::algorithm::equals(name, "IEC:DB")) {
                    auto const db = reader[name].get("db", "default");
                    auto const pos = sm.find(db);
                    if (pos != sm.end()) {
                        start_iec_db(
                            ctl, channels, logger, pos->second, cyng::container_cast<cyng::param_map_t>(reader.get(name)), name);
                    } else {
                        CYNG_LOG_FATAL(logger, "no database [" << db << "] for writer " << name << " configured");
                    }
                } else if (boost::algorithm::equals(name, "SML:XML")) {
                } else if (boost::algorithm::equals(name, "SML:JSON")) {
                    start_iec_json(
                        ctl,
                        channels,
                        logger,
                        name,
                        cyng::value_cast(reader[name]["root-dir"].get(), tmp.string()),
                        cyng::value_cast(reader[name]["prefix"].get(), "iec"),
                        cyng::value_cast(reader[name]["suffix"].get(), "json"));
                } else if (boost::algorithm::equals(name, "IEC:LOG")) {
                    start_iec_log(
                        ctl,
                        channels,
                        logger,
                        name,
                        cyng::value_cast(reader[name]["root-dir"].get(), tmp.string()),
                        cyng::value_cast(reader[name]["prefix"].get(), "iec"),
                        cyng::value_cast(reader[name]["suffix"].get(), "log"));
                } else if (boost::algorithm::equals(name, "IEC:CSV")) {
                    start_iec_csv(
                        ctl,
                        channels,
                        logger,
                        name,
                        cyng::value_cast(reader[name]["root-dir"].get(), tmp.string()),
                        cyng::value_cast(reader[name]["prefix"].get(), "sml"),
                        cyng::value_cast(reader[name]["suffix"].get(), "csv"),
                        cyng::value_cast(reader[name]["header"].get(), true));
                } else if (boost::algorithm::equals(name, "IEC:influxdb")) {
                    start_iec_influx(
                        ctl,
                        channels,
                        logger,
                        name,
                        cyng::value_cast(reader[name]["host"].get(), "localhost"),
                        cyng::value_cast(reader[name]["port"].get(), "8086"),
                        cyng::value_cast(reader[name]["protocol"].get(), "http"),
                        cyng::value_cast(reader[name]["cert"].get(), "cert.pem"),
                        cyng::value_cast(reader[name]["db"].get(), "SMF"));
                } else if (boost::algorithm::equals(name, "DLMS:influxdb")) {
                    start_dlms_influx(
                        ctl,
                        channels,
                        logger,
                        name,
                        cyng::value_cast(reader[name]["host"].get(), "localhost"),
                        cyng::value_cast(reader[name]["port"].get(), "8086"),
                        cyng::value_cast(reader[name]["protocol"].get(), "http"),
                        cyng::value_cast(reader[name]["cert"].get(), "cert.pem"),
                        cyng::value_cast(reader[name]["db"].get(), "SMF"));
                } else {
                    CYNG_LOG_WARNING(logger, "unknown writer task: " << name);
                }
            }

        } else {
            CYNG_LOG_FATAL(logger, "no writer tasks configured");
        }

        //
        // start csv reports
        //
        {
            cfg::csv_report const cfg(cyng::container_cast<cyng::param_map_t>(reader.get("csv.reports")));
            auto const db = cfg.get_db();
            auto const pos = sm.find(db);
            if (pos != sm.end()) {
                start_csv_reports(ctl, channels, logger, pos->second, cfg);
            } else {
                CYNG_LOG_FATAL(logger, "no database [" << db << "] for CSV reports configured");
            }
        }

        //
        // start LPEx reports
        //
        {
            cfg::lpex_report const cfg(cyng::container_cast<cyng::param_map_t>(reader.get("lpex.reports")));
            auto const db = cfg.get_db();
            auto const pos = sm.find(db);
            if (pos != sm.end()) {
                start_lpex_reports(ctl, channels, logger, pos->second, cfg);
            } else {
                CYNG_LOG_FATAL(logger, "no database [" << db << "] for CSV reports configured");
            }
        }

        //
        // start feed reports
        //
        {
            cfg::feed_report const cfg(cyng::container_cast<cyng::param_map_t>(reader.get("feed.reports")));
            auto const db = reader["feed.reports"].get("db", "default");
            auto const pos = sm.find(db);
            if (pos != sm.end()) {
                start_feed_reports(ctl, channels, logger, pos->second, cfg);
            } else {
                CYNG_LOG_FATAL(logger, "no database [" << db << "] for feed reports configured");
            }
        }

        //  No duplicates possible by using a set
        auto const target_sml = cyng::set_cast<std::string>(reader["targets"]["SML"].get(), "sml@store");
        auto const target_iec = cyng::set_cast<std::string>(reader["targets"]["IEC"].get(), "iec@store");
        auto const target_dlms = cyng::set_cast<std::string>(reader["targets"]["DLMS"].get(), "dlms@store");

        if (target_sml.empty()) {
            CYNG_LOG_WARNING(logger, "no SML targets configured");
        } else {
            CYNG_LOG_TRACE(logger, target_sml.size() << " SML targets configured");
        }
        if (target_iec.empty()) {
            CYNG_LOG_WARNING(logger, "no IEC targets configured");
        } else {
            CYNG_LOG_TRACE(logger, target_iec.size() << " IEC targets configured");
        }
        if (target_dlms.empty()) {
            CYNG_LOG_WARNING(logger, "no DLMS targets configured");
        } else {
            CYNG_LOG_TRACE(logger, target_dlms.size() << " DLMS targets configured");
        }

        //
        //  seconds to wait before starting ip-t client
        //
        auto const delay = cyng::to_seconds(reader.get("network.delay", "00:00:06"));
        CYNG_LOG_INFO(logger, "start ipt bus in " << delay << " seconds");

        //
        //  connect to ip-t server
        //
        join_network(
            ctl, channels, logger, tag, node_name, model, std::move(tgl), delay, target_sml, target_iec, target_dlms, writer);
    }

    void controller::start_cleanup_tasks(
        cyng::controller &ctl,
        cyng::logger logger,
        std::string name,
        cyng::db::session db,
        cyng::param_map_t &&cleanup_tasks) {

        //
        //  first vacuum dabase
        //
        CYNG_LOG_INFO(logger, "start vacuum database");
        vacuum(db);
        CYNG_LOG_TRACE(logger, "vacuum database complete");

        //
        //  read configuration for "cleanup"
        //
        cfg::cleanup const cfg(std::move(cleanup_tasks));
        auto const profiles = cfg.get_profiles();
        for (auto const &code : profiles) {
            if (cfg.is_enabled(code)) {
                BOOST_ASSERT(sml::is_profile(code));
                auto const name = cfg.get_name(code);
                auto const max_age = cfg.get_max_age(code);

                auto channel = ctl.create_named_channel_with_ref<cleanup_db>("cleanup.db", ctl, logger, db, code, max_age).first;
                BOOST_ASSERT(channel->is_open());

                // don't start immediately and earliest after one hour.
                // calculate delay:
                auto const delay = ((max_age / 2) < std::chrono::hours(1)) ? std::chrono::hours(1) : (max_age / 2);
                channel->suspend(
                    delay,
                    "run",
                    //  handle dispatch errors
                    std::bind(cyng::log_dispatch_error, logger, std::placeholders::_1, std::placeholders::_2));

                CYNG_LOG_INFO(
                    logger,
                    "start db cleanup task \"" << name << "\" for profile " << obis::get_name(code) << " in " << delay << " hours");

            } else {
                CYNG_LOG_WARNING(
                    logger, "db cleanup task " << name << " for profile " << obis::get_name(code) << " is not enabled");
            }
        }
    }

    void controller::start_gap_reports(
        cyng::controller &ctl,
        cyng::logger logger,
        std::string name,
        cyng::db::session db,
        cyng::param_map_t &&gap_tasks) {

        //
        //  read configuration for "gap"
        //
        cfg::gap const cfg(std::move(gap_tasks));
        auto const profiles = cfg.get_profiles();
        for (auto const &code : profiles) {
            if (cfg.is_enabled(code)) {
                BOOST_ASSERT(sml::is_profile(code));
                auto const name = cfg.get_name(code);
                auto const path = cfg.get_path(code);
                if (sanitize_path(path)) {
                    auto const backtrack = cfg.get_backtrack(code);

                    CYNG_LOG_TRACE(logger, "gap report " << name << " path is " << path);

                    auto channel =
                        ctl.create_named_channel_with_ref<gap_report>("gap.report", ctl, logger, db, code, path, backtrack).first;
                    BOOST_ASSERT(channel->is_open());

                    channel->dispatch(
                        "run",
                        //  handle dispatch errors
                        std::bind(cyng::log_dispatch_error, logger, std::placeholders::_1, std::placeholders::_2),
                        std::chrono::hours(48));
                } else {
                    CYNG_LOG_WARNING(logger, "[gap.report] cannot create path " << path);
                }
            } else {
                CYNG_LOG_WARNING(
                    logger, "gap report on db " << name << " for profile " << obis::get_name(code) << " is not enabled");
            }
        }
    }

    void controller::shutdown(cyng::registry &reg, cyng::stash &channels, cyng::logger logger) {

        // config::stop_tasks(logger, reg, "network");

        channels.stop();
        channels.clear();
    }

    void controller::join_network(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        boost::uuids::uuid tag,
        std::string const &node_name,
        std::string const &model,
        ipt::toggle::server_vec_t &&tgl,
        std::chrono::seconds delay,
        std::set<std::string> const &sml_targets,
        std::set<std::string> const &iec_targets,
        std::set<std::string> const &dlms_targets,
        std::set<std::string> const &writer) {

        auto [channel, tsk] = ctl.create_named_channel_with_ref<network>(
            "network", ctl, tag, logger, node_name, model, std::move(tgl), sml_targets, iec_targets, dlms_targets, writer);
        BOOST_ASSERT(channel->is_open());
        channels.lock(channel);
        BOOST_ASSERT(tsk != nullptr);

        //
        //  "connect" is also as slot available
        //
        tsk->connect();
    }

    bool controller::run_options(boost::program_options::variables_map &vars) {

        if (vars["init"].as<bool>()) {
            //	initialize database

            //
            //  read configuration
            //
            auto const cfg = read_config_section(config_.json_path_, config_.config_index_);
            auto sm = init_storage(cfg, true);
            auto const reader = cyng::make_reader(cfg);

            auto writer = cyng::set_cast<std::string>(reader["writer"].get(), "ALL:BIN");
            for (std::string const name : writer) {
                if (boost::algorithm::equals(name, "SML:DB")) {
                    auto const db = reader[name].get("db", "default");
                    auto const pos = sm.find(db);
                    if (pos != sm.end()) {
                        std::cout << "***info : writer " << name << " initialize database " << pos->first << std::endl;
                        sml_db_writer::init_storage(pos->second);
                    } else {
                        std::cerr << "***error: writer " << name << " uses an undefined database: " << db << std::endl;
                    }
                } else if (boost::algorithm::equals(name, "IEC:DB")) {
                    auto const db = reader[name].get("db", "default");
                    auto const pos = sm.find(db);
                    if (pos != sm.end()) {
                        std::cout << "***info : writer " << name << " initialize database " << pos->first << std::endl;
                        iec_db_writer::init_storage(pos->second);
                    } else {
                        std::cerr << "***error: writer " << name << " uses an undefined database: " << db << std::endl;
                    }
                }
            }

            //
            //  close database
            //
            for (auto s : sm) {
                if (s.second.is_alive()) {
                    s.second.close();
                }
            }
            return true;
        }

        //
        //	execute a influx db command
        //
        auto const cmd = vars["influxdb"].as<std::string>();
        if (!cmd.empty()) {
            auto const cfg = read_config_section(config_.json_path_, config_.config_index_);
            // std::cerr << cmd << " not implemented yet" << std::endl;
            // return ctrl.create_influx_dbs(config_index, cmd);
            create_influx_dbs(cfg, cmd);
            return true;
        }

        //
        //  generate reports
        //
        if (!vars["generate"].defaulted()) {
            //	generate all reports
            auto const type = vars["generate"].as<std::string>();
            auto const cfg = read_config_section(config_.json_path_, config_.config_index_);
            auto sm = init_storage(cfg, false);
            auto const reader = cyng::make_reader(cfg);

            //  ToDo: select section from cfg and report if missing
            if (boost::algorithm::equals(type, "csv")) {
                generate_csv_reports(sm, cyng::container_cast<cyng::param_map_t>(reader.get("csv.reports")));
            } else if (boost::algorithm::equals(type, "lpex")) {
                generate_lpex_reports(sm, cyng::container_cast<cyng::param_map_t>(reader.get("lpex.reports")));
            } else if (boost::algorithm::equals(type, "gap")) {
                generate_gap_reports(read_config_section(config_.json_path_, config_.config_index_));
            } else if (boost::algorithm::equals(type, "feed") || boost::algorithm::equals(type, "adv")) {
                generate_feed_reports(sm, cyng::container_cast<cyng::param_map_t>(reader.get("feed.reports")));
            }
            return true; //  stop application
        }

        if (vars["cleanup"].as<bool>()) {
            //  remove outdated records
            cleanup_archive(read_config_section(config_.json_path_, config_.config_index_));
            return true; //  stop application
        }

        //
        //  dump readout data
        //
        if (!vars["dump"].defaulted()) {
            //	generate all reports
            auto const hours = std::chrono::hours(vars["dump"].as<int>() * 24);
            dump_readout(read_config_section(config_.json_path_, config_.config_index_), hours);
            return true; //  stop application
        }

        //
        //	call base classe
        //
        return controller_base::run_options(vars);
    }

    void controller::generate_csv_reports(std::map<std::string, cyng::db::session> &sm, cyng::param_map_t &&pm) {

        //     "csv.reports": {
        //          "db": ...,
        //          "profiles": {
        //              "8181c78611ff": { ... }
        //          }
        //      }

        cfg::csv_report cfg(std::move(pm));

        //  reference to "db" section
        auto const db = cfg.get_db();
        auto const pos = sm.find(db);
        if (pos != sm.end()) {

            if (pos->second.is_alive()) {
                auto const now = cyng::make_utc_date();

                //
                //  csv.reports/profiles
                //  get set of all specified profiles
                //
                auto const profiles = cfg.get_profiles();
                for (auto const &code : profiles) {

                    auto const name = cfg.get_name(code);

                    if (cfg.is_enabled(code)) {
                        std::cout << "***info: generate csv report " << name << " (" << code << ")" << std::endl;

                        auto const path = cfg.get_path(code);
                        if (sanitize_path(path)) {
                            auto const backtrack = cfg.get_backtrack(code);
                            auto const prefix = cfg.get_prefix(code);
                            generate_csv(pos->second, code, path, prefix, now, backtrack);
                        } else {
                            std::cout << "***error: output path [" << path << "] of csv report " << name << " does not exists"
                                      << std::endl;
                        }
                    } else {
                        std::cout << "***info: csv report " << name << " is disabled" << std::endl;
                    }
                }
            }
        } else {
            std::cerr << "***error: csv report uses an undefined database: " << db << std::endl;
        }
    }

    void controller::generate_lpex_reports(std::map<std::string, cyng::db::session> &sm, cyng::param_map_t &&pm) {

        //     "lpex.reports": {
        //          "print.version": true,
        //          "debug": true,
        //          "filter": "0100010800ff:0100010801ff:0700030000ff",
        //          "profiles": {
        //              "8181c78611ff": { ... }
        //          }
        //      }

        cfg::lpex_report cfg(std::move(pm));

        auto const db = cfg.get_db();
        auto const pos = sm.find(db);
        if (pos != sm.end()) {

            if (pos->second.is_alive()) {
                // auto const cwd = std::filesystem::current_path();
                auto const now = cyng::make_utc_date();

                //  lpex.reports/print.version
                auto const print_version = cfg.is_print_version();

                //  lpex.reports/debug
                auto const debug_mode = cfg.is_debug_mode();

                //  lpex.reports/filter
                auto const filter = cfg.get_filter();

                auto const profiles = cfg.get_profiles();
                for (auto const &code : profiles) {

                    auto const name = cfg.get_name(code);

                    if (cfg.is_enabled(code)) {
                        std::cout << "***info: generate lpex report " << name << " (" << code << ")" << std::endl;
                        auto const path = cfg.get_path(code);
                        if (sanitize_path(path)) {
                            auto const backtrack = cfg.get_backtrack(code);
                            auto const customer = cfg.add_customer_data(code);
                            auto const prefix = cfg.get_prefix(code);
                            generate_lpex(
                                pos->second, code, filter, path, prefix, now, backtrack, print_version, debug_mode, customer);
                        } else {
                            std::cout << "***error: output path [" << path << "] of lpex report " << name << " does not exists"
                                      << std::endl;
                        }
                    } else {
                        std::cout << "***info: lpex report " << name << " is disabled" << std::endl;
                    }
                }
            }
        } else {
            std::cerr << "***error: lpex report uses an undefined database: " << db << std::endl;
        }
    }

    void controller::generate_gap_reports(cyng::object &&cfg) {
        auto const now = cyng::make_utc_date();

        //
        //  data base connections
        //
        auto sm = init_storage(cfg, false);
        auto const reader = cyng::make_reader(std::move(cfg));

        auto const pm = cyng::container_cast<cyng::param_map_t>(reader.get("db"));
        for (auto const &param : pm) {
            auto const db = cyng::container_cast<cyng::param_map_t>(param.second);
            auto s = cyng::db::create_db_session(db);
            if (s.is_alive()) {
                std::cout << "generate gap report for db [" << param.first << "]" << std::endl;
                auto const gap_tasks = cyng::container_cast<cyng::param_map_t>(reader["db"][param.first].get("gap"));
                for (auto const &tsk : gap_tasks) {
                    auto const reader_gap = cyng::make_reader(tsk.second);
                    auto const profile = cyng::to_obis(tsk.first);
                    auto const enabled = reader_gap.get("enabled", false);
                    if (enabled) {
                        auto const age = cyng::to_hours(reader_gap.get("backtrack", "48:00:00"));

                        // auto const d = cyng::date::make_date_from_local_time(now - age);
                        std::cout << "the gap report on db \"" << param.first << "\" for profile " << obis::get_name(profile)
                                  << " start with " << cyng::as_string(now, "%Y-%m-%d %T") << std::endl;
                        auto const root = reader_gap.get("path", "");
                        if (!std::filesystem::exists(root)) {
                            std::cout << "***warning: output path [" << root << "] of gap report " << obis::get_name(profile)
                                      << " does not exists";
                            std::error_code ec;
                            if (!std::filesystem::create_directories(root, ec)) {
                                std::cerr << "***error: cannot create path [" << root << "]: " << ec.message();
                            }
                        }
                        smf::generate_gap(s, profile, root, now, age);
                    } else {
                        std::cout << "gap report on db " << param.first << " for profile " << obis::get_name(profile)
                                  << " is disabled" << std::endl;
                    }
                }
            } else {
                std::cout << "***warning: database [" << param.first << "] is not reachable" << std::endl;
            }
        }
    }

    void controller::generate_feed_reports(std::map<std::string, cyng::db::session> &sm, cyng::param_map_t &&pm) {

        //     "feed.reports": {
        //          "print.version": true,
        //          "debug": true,
        //          "filter": "0100010800ff:0100010801ff:0700030000ff",
        //          "profiles": {
        //              "8181c78611ff": { ... }
        //          }
        //      }

        cfg::feed_report cfg(std::move(pm));

        //  reference to "db" section
        auto const db = cfg.get_db();
        auto const pos = sm.find(db);
        if (pos != sm.end()) {

            if (pos->second.is_alive()) {
                auto const now = cyng::make_utc_date();

                //  feed.reports/print.version
                auto const print_version = cfg.is_print_version();

                //  feed.reports/debug
                auto const debug_mode = cfg.is_debug_mode();

                //
                //  get set of all specified profiles
                //
                auto const profiles = cfg.get_profiles();
                for (auto const &code : profiles) {
                    auto const name = cfg.get_name(code);
                    if (cfg.is_enabled(code)) {
                        std::cout << "***info: generate feed report " << name << " (" << code << ")" << std::endl;
                        auto const path = cfg.get_path(code);
                        if (sanitize_path(path)) {
                            auto const backtrack = cfg.get_backtrack(code);
                            auto const customer = cfg.add_customer_data(code);
                            auto const prefix = cfg.get_prefix(code);
                            auto const shift_factor = cfg.get_shift_factor(code);
                            generate_feed(
                                pos->second, code, path, prefix, now, backtrack, print_version, debug_mode, customer, shift_factor);

                        } else {
                            std::cout << "***error: output path [" << path << "] of feed report " << name << " does not exists"
                                      << std::endl;
                        }
                    } else {
                        std::cout << "***info: feed report " << name << " is disabled" << std::endl;
                    }
                }
            }
        } else {
            std::cerr << "***error: feed report uses an undefined database: " << db << std::endl;
        }
    }

    void controller::cleanup_archive(cyng::object &&cfg) {
        auto const now = cyng::make_utc_date();
        auto const reader = cyng::make_reader(cfg);
        BOOST_ASSERT(reader.get("db").tag() == cyng::TC_PARAM_MAP);
        auto const pm = cyng::container_cast<cyng::param_map_t>(reader.get("db"));
        bool vac = false;
        for (auto const &param : pm) {
            auto const db = cyng::container_cast<cyng::param_map_t>(param.second);
            auto s = cyng::db::create_db_session(db);
            if (s.is_alive()) {
                std::cout << "cleanup database [" << param.first << "]" << std::endl;
                auto const cleanup_tasks = cyng::container_cast<cyng::param_map_t>(reader["db"][param.first].get("cleanup"));
                for (auto const &tsk : cleanup_tasks) {
                    auto const reader_cls = cyng::make_reader(tsk.second);
                    auto const profile = cyng::to_obis(tsk.first);
                    auto const enabled = reader_cls.get("enabled", false);
                    if (!vac) {
                        vac = true;
                        std::cout << "vacuum database" << std::endl;
                        // smf::vacuum(s);
                    }
                    if (enabled) {
                        auto const age = cyng::to_hours(reader_cls.get("max.age", "48:00:00"));

                        auto const d = now - age;
                        std::cout << "start cleanup task on db \"" << param.first << "\" for profile " << obis::get_name(profile)
                                  << " older than " << cyng::as_string(d, "%Y-%m-%d %H:%M") << std::endl;
                        // auto const limit = reader_cls.get("limit", 256u);
                        smf::cleanup(s, profile, d);
                        std::cout << "profile " << obis::get_name(profile) << " complete" << std::endl;
                    } else {
                        std::cout << "cleanup task on db " << param.first << " for profile " << obis::get_name(profile)
                                  << " is disabled" << std::endl;
                    }
                }
            } else {
                std::cout << "***warning: database [" << param.first << "] is not reachable" << std::endl;
            }
        }
    }

    void controller::dump_readout(cyng::object &&cfg, std::chrono::hours backlog) {
        auto const now = cyng::make_utc_date();
        auto const reader = cyng::make_reader(cfg);
        BOOST_ASSERT(reader.get("db").tag() == cyng::TC_PARAM_MAP);
        auto const pm = cyng::container_cast<cyng::param_map_t>(reader.get("db"));
        for (auto const &param : pm) {
            auto const db = cyng::container_cast<cyng::param_map_t>(param.second);
            auto s = cyng::db::create_db_session(db);
            if (s.is_alive()) {
                smf::dump_readout(s, now.get_end_of_day(), backlog);
            } else {
                std::cout << "***warning: database [" << param.first << "] is not reachable" << std::endl;
            }
        }
    }

    std::map<std::string, cyng::db::session> controller::init_storage(cyng::object const &cfg, bool create) {

        std::map<std::string, cyng::db::session> sm;
        auto const reader = cyng::make_reader(cfg);
        BOOST_ASSERT(reader.get("db").tag() == cyng::TC_PARAM_MAP);
        auto const pm = cyng::container_cast<cyng::param_map_t>(reader.get("db"));
        for (auto const &param : pm) {
            auto const db = cyng::container_cast<cyng::param_map_t>(param.second);
            if (create) {
                std::cout << "create database [" << param.first << "]" << std::endl;
                sm.emplace(param.first, init_storage(db));
            } else {
                sm.emplace(param.first, cyng::db::create_db_session(db));
            }
        }
        return sm;
    }

    cyng::db::session controller::init_storage(cyng::param_map_t const &pm) {
        auto s = cyng::db::create_db_session(pm);
        if (s.is_alive()) {
            iec_db_writer::init_storage(s); //	see task/iec_db_writer.h
            sml_db_writer::init_storage(s); //	see task/sml_db_writer.h
            std::cout << "***info : database created" << std::endl;
        } else {
            std::cerr << "***error: create database failed: " << pm << std::endl;
        }
        return s;
    }

    int controller::create_influx_dbs(cyng::object const &cfg, std::string const &cmd) {

        auto const reader = cyng::make_reader(cfg);
        auto const pmap = cyng::container_cast<cyng::param_map_t>(reader.get("SML:influxdb"));
        std::cout << "***info: " << pmap << std::endl;

        cyng::controller ctl(config_.pool_size_);
        int rc = EXIT_FAILURE;

        if (boost::algorithm::equals(cmd, "create")) {

            //
            //	create database
            //
            rc = influx::create_db(
                ctl.get_ctx(),
                std::cout,
                cyng::value_cast(reader["SML:influxdb"]["host"].get(), "localhost"),
                cyng::value_cast(reader["SML:influxdb"]["port"].get(), "8086"),
                cyng::value_cast(reader["SML:influxdb"]["protocol"].get(), "http"),
                cyng::value_cast(reader["SML:influxdb"]["cert"].get(), "cert.pem"),
                cyng::value_cast(reader["SML:influxdb"]["db"].get(), "SMF"));
        } else if (boost::algorithm::equals(cmd, "show")) {
            //
            //	show database
            //

            rc = influx::show_db(
                ctl.get_ctx(),
                std::cout,
                cyng::value_cast(reader["SML:influxdb"]["host"].get(), "localhost"),
                cyng::value_cast(reader["SML:influxdb"]["port"].get(), "8086"),
                cyng::value_cast(reader["SML:influxdb"]["protocol"].get(), "http"),
                cyng::value_cast(reader["SML:influxdb"]["cert"].get(), "cert.pem"));

        } else if (boost::algorithm::equals(cmd, "drop")) {
            //
            //	drop database
            //
            rc = influx::drop_db(
                ctl.get_ctx(),
                std::cout,
                cyng::value_cast(reader["SML:influxdb"]["host"].get(), "localhost"),
                cyng::value_cast(reader["SML:influxdb"]["port"].get(), "8086"),
                cyng::value_cast(reader["SML:influxdb"]["protocol"].get(), "http"),
                cyng::value_cast(reader["SML:influxdb"]["cert"].get(), "cert.pem"),
                cyng::value_cast(reader["SML:influxdb"]["db"].get(), "SMF"));
        } else {
            std::cerr << "***error: unknown command " << cmd << std::endl;
        }

        ctl.cancel();
        ctl.stop();
        return rc;
    }

    void controller::start_sml_db(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::db::session db,
        std::string const &name,
        std::set<cyng::obis> const &exclude) {

        CYNG_LOG_INFO(logger, "start sml database writer");
        auto channel = ctl.create_named_channel_with_ref<sml_db_writer>(name, ctl, logger, db, exclude).first;
        BOOST_ASSERT(channel->is_open());
        channels.lock(channel);
    }
    void controller::start_sml_xml(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        std::string const &name,
        std::string out,
        std::string prefix,
        std::string suffix) {

        if (!std::filesystem::exists(out)) {
            std::error_code ec;
            std::filesystem::create_directories(out, ec);
            if (ec) {
                CYNG_LOG_ERROR(logger, "[sml.xml.writer] " << ec.message());
                return; //  give up
            }
        } else {
            CYNG_LOG_INFO(logger, "[sml.xml.writer] start -> " << out);
        }
        if (!out.empty()) {
            CYNG_LOG_INFO(logger, "start sml xml writer");
            auto channel = ctl.create_named_channel_with_ref<sml_xml_writer>(name, ctl, logger, out, prefix, suffix).first;
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }
    void controller::start_sml_json(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        std::string const &name,
        std::string out,
        std::string prefix,
        std::string suffix) {

        if (!std::filesystem::exists(out)) {
            std::error_code ec;
            std::filesystem::create_directories(out, ec);
            if (ec) {
                CYNG_LOG_ERROR(logger, "[sml.json.writer] " << ec.message());
                return; //  give up
            }
        } else {
            CYNG_LOG_INFO(logger, "[sml.json.writer] start -> " << out);
        }

        if (!out.empty()) {
            auto channel = ctl.create_named_channel_with_ref<sml_json_writer>(name, ctl, logger, out, prefix, suffix).first;
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }
    void controller::start_sml_abl(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        std::string const &name,
        std::string out,
        std::string prefix,
        std::string suffix,
        bool eol_dos) {

        if (!out.empty()) {
            if (!std::filesystem::exists(out)) {
                std::error_code ec;
                std::filesystem::create_directories(out, ec);
                if (ec) {
                    CYNG_LOG_ERROR(logger, "[sml.abl.writer] " << ec.message());
                    return; //  give up
                }
            } else {
                CYNG_LOG_INFO(logger, "[sml.abl.writer] start -> " << out);
            }
            auto channel = ctl.create_named_channel_with_ref<sml_abl_writer>(name, ctl, logger, out, prefix, suffix, eol_dos).first;
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        } else {
            CYNG_LOG_ERROR(logger, "[sml.abl.writer] missing output path");
        }
    }
    void controller::start_sml_log(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        std::string const &name,
        std::string out,
        std::string prefix,
        std::string suffix,
        bool header) {
        // CYNG_LOG_INFO(logger, "start sml log writer -> " << out);
        if (!out.empty()) {
            if (!std::filesystem::exists(out)) {
                std::error_code ec;
                std::filesystem::create_directories(out, ec);
                if (ec) {
                    CYNG_LOG_ERROR(logger, "[sml.log.writer] " << ec.message());
                    return; //  give up
                }
            } else {
                CYNG_LOG_INFO(logger, "[sml.log.writer] start -> " << out);
            }
            auto channel = ctl.create_named_channel_with_ref<sml_log_writer>(name, ctl, logger, out, prefix, suffix).first;
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }
    void controller::start_sml_csv(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        std::string const &name,
        std::string out,
        std::string prefix,
        std::string suffix,
        bool header) {

        if (!out.empty()) {
            if (!std::filesystem::exists(out)) {
                std::error_code ec;
                std::filesystem::create_directories(out, ec);
                if (ec) {
                    CYNG_LOG_ERROR(logger, "[sml.csv.writer] " << ec.message());
                    return; //  give up
                }
            } else {
                CYNG_LOG_INFO(logger, "[sml.csv.writer] start -> " << out);
            }

            auto channel = ctl.create_named_channel_with_ref<sml_csv_writer>(name, ctl, logger, out, prefix, suffix, header).first;
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        } else {
            CYNG_LOG_ERROR(logger, "[sml.csv.writer] missing output path");
        }
    }
    void controller::start_sml_influx(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        std::string const &name,
        std::string const &host,
        std::string const &service,
        std::string const &protocol,
        std::string const &cert,
        std::string const &db) {

        CYNG_LOG_INFO(logger, "start " << name);
        auto channel =
            ctl.create_named_channel_with_ref<sml_influx_writer>(name, ctl, logger, host, service, protocol, cert, db).first;
        BOOST_ASSERT(channel->is_open());
        channels.lock(channel);
    }

    void controller::start_iec_db(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::db::session db,
        cyng::param_map_t &&pm,
        std::string const &name) {
        if (!pm.empty()) {
            CYNG_LOG_INFO(logger, "start iec database writer");
            auto channel = ctl.create_named_channel_with_ref<iec_db_writer>(name, ctl, logger, db).first;
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }
    void controller::start_iec_json(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        std::string const &name,
        std::string out,
        std::string prefix,
        std::string suffix) {

        if (!std::filesystem::exists(out)) {
            std::error_code ec;
            std::filesystem::create_directories(out, ec);
            if (ec) {
                CYNG_LOG_ERROR(logger, "[iec.json.writer] " << ec.message());
                return; //  give up
            }
        } else {
            CYNG_LOG_INFO(logger, "[iec.json.writer] start -> " << out);
        }

        if (!out.empty()) {
            auto channel = ctl.create_named_channel_with_ref<iec_json_writer>(name, ctl, logger, out, prefix, suffix).first;
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }

    void controller::start_iec_log(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        std::string const &name,
        std::string out,
        std::string prefix,
        std::string suffix) {
        CYNG_LOG_INFO(logger, "start iec log writer -> " << out);
        if (!out.empty()) {
            auto channel = ctl.create_named_channel_with_ref<iec_log_writer>(name, ctl, logger, out, prefix, suffix).first;
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }
    void controller::start_iec_csv(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        std::string const &name,
        std::string out,
        std::string prefix,
        std::string suffix,
        bool header) {
        CYNG_LOG_INFO(logger, "start iec csv writer -> " << out);
        if (!out.empty()) {
            auto channel = ctl.create_named_channel_with_ref<iec_csv_writer>(name, ctl, logger, out, prefix, suffix, header).first;
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }
    void controller::start_iec_influx(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        std::string const &name,
        std::string const &host,
        std::string const &service,
        std::string const &protocol,
        std::string const &cert,
        std::string const &db) {
        CYNG_LOG_INFO(logger, "start " << name);
        auto channel =
            ctl.create_named_channel_with_ref<iec_influx_writer>(name, ctl, logger, host, service, protocol, cert, db).first;
        BOOST_ASSERT(channel->is_open());
        channels.lock(channel);
    }
    void controller::start_dlms_influx(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        std::string const &name,
        std::string const &host,
        std::string const &service,
        std::string const &protocol,
        std::string const &cert,
        std::string const &db) {
        CYNG_LOG_INFO(logger, "start " << name);
        auto channel =
            ctl.create_named_channel_with_ref<dlms_influx_writer>(name, ctl, logger, host, service, protocol, cert, db).first;
        BOOST_ASSERT(channel->is_open());
        channels.lock(channel);
    }

    void controller::start_csv_reports(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::db::session db,
        cfg::csv_report const &cfg) {

        //
        //	start reporting
        //
        auto const profiles = cfg.get_profiles();
        for (auto const &code : profiles) {
            auto const name = cfg.get_name(code);
            if (cfg.is_enabled(code)) {

                BOOST_ASSERT(sml::is_profile(code));
                auto const path = cfg.get_path(code);
                if (sanitize_path(path)) {
                    auto const backtrack = cfg.get_backtrack(code);
                    auto const prefix = cfg.get_prefix(code);

                    CYNG_LOG_TRACE(logger, "CSV report " << name << " path is " << path << " - " << prefix);

                    auto channel =
                        ctl.create_named_channel_with_ref<csv_report>(name, ctl, logger, db, code, path, backtrack, prefix).first;
                    BOOST_ASSERT(channel->is_open());
                    channels.lock(channel);

                    //
                    //  calculate start time
                    //
                    auto const now = cyng::make_utc_date();
                    auto const interval = sml::interval_time(now, code);
                    CYNG_LOG_INFO(logger, "start CSV report " << code << " (" << name << ") at " << now + interval);

                    //  handle dispatch errors
                    channel->suspend(
                        interval, "run", std::bind(cyng::log_dispatch_error, logger, std::placeholders::_1, std::placeholders::_2));
                } else {
                    CYNG_LOG_ERROR(logger, "CSV output path [" << path << "] of report " << name << " does not exists");
                }

            } else {
                CYNG_LOG_TRACE(logger, "CSV report " << name << " is disabled");
            }
        }
    }

    void controller::start_lpex_reports(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::db::session db,
        cfg::lpex_report const &cfg) {

        //
        //	start LPEx reporting
        //
        auto const now = cyng::make_utc_date();
        auto const filter = cfg.get_filter();
        auto const debug_mode = cfg.is_debug_mode();
        auto const print_version = cfg.is_print_version();

        auto const profiles = cfg.get_profiles();
        for (auto const &code : profiles) {
            auto const name = cfg.get_name(code);
            if (cfg.is_enabled(code)) {

                BOOST_ASSERT(sml::is_profile(code));
                auto const path = cfg.get_path(code);
                if (sanitize_path(path)) {
                    auto const backtrack = cfg.get_backtrack(code);
                    auto const prefix = cfg.get_prefix(code);
                    auto const customer = cfg.add_customer_data(code);

                    CYNG_LOG_TRACE(logger, "LPEx report " << name << " path is " << path << " - " << prefix);

                    auto channel =
                        ctl.create_named_channel_with_ref<lpex_report>(
                               name, ctl, logger, db, code, filter, path, backtrack, prefix, print_version, debug_mode, customer)
                            .first;
                    BOOST_ASSERT(channel->is_open());
                    channels.lock(channel);

                    //
                    //  calculate start time
                    //
                    auto const interval = sml::interval_time(now, code);

                    //
                    //  make sure that the interval is at least 2h long
                    //
                    auto const delay = (interval < std::chrono::minutes(2 * 60)) ? std::chrono::minutes(2 * 60) : interval;
                    CYNG_LOG_INFO(
                        logger, "start LPEx report " << code << " (" << name << ") at " << cyng::as_string(now + delay, "%F %T%z"));
                    channel->suspend(
                        delay,
                        "run",
                        //  handle dispatch errors
                        std::bind(cyng::log_dispatch_error, logger, std::placeholders::_1, std::placeholders::_2));
                } else {
                    CYNG_LOG_ERROR(logger, "LPEx output path [" << path << "] of report " << name << " does not exist");
                }
            } else {
                CYNG_LOG_TRACE(logger, "LPEx report " << code << " (" << name << ") is disabled");
            }
        }
    }

    void controller::start_feed_reports(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::db::session db,
        cfg::feed_report const &cfg) {

        //
        //	start feed reporting
        //
        auto const now = cyng::make_utc_date();
        auto const debug_mode = cfg.is_debug_mode();
        auto const print_version = cfg.is_print_version();

        auto const profiles = cfg.get_profiles();
        for (auto const &code : profiles) {
            auto const name = cfg.get_name(code);
            if (cfg.is_enabled(code)) {

                BOOST_ASSERT(sml::is_profile(code));
                auto const path = cfg.get_path(code);
                if (sanitize_path(path)) {
                    auto const backtrack = cfg.get_backtrack(code);
                    auto const prefix = cfg.get_prefix(code);
                    auto const customer = cfg.add_customer_data(code);
                    auto const shift_factor = cfg.get_shift_factor(code);

                    CYNG_LOG_TRACE(logger, "feed/adv report " << name << " path is " << path << " - " << prefix);

                    auto channel = ctl.create_named_channel_with_ref<feed_report>(
                                          name,
                                          ctl,
                                          logger,
                                          db,
                                          code,
                                          path,
                                          backtrack,
                                          prefix,
                                          print_version,
                                          debug_mode,
                                          customer,
                                          shift_factor)
                                       .first;
                    BOOST_ASSERT(channel->is_open());
                    channels.lock(channel);

                    //
                    //  calculate start time
                    //
                    auto const interval = sml::interval_time(now, code);

#ifdef _DEBUG
                    // debug mode only
                    auto const delay = std::chrono::minutes(10);
#else
                    //
                    //  make sure that the interval is at least 2h long
                    //
                    auto const delay = (interval < std::chrono::minutes(2 * 60)) ? std::chrono::minutes(2 * 60) : interval;
#endif
                    CYNG_LOG_INFO(
                        logger,
                        "start feed/adv report " << code << " (" << name << ") at " << cyng::as_string(now + delay, "%F %T%z"));
                    channel->suspend(
                        delay,
                        "run",
                        //  handle dispatch errors
                        std::bind(cyng::log_dispatch_error, logger, std::placeholders::_1, std::placeholders::_2));
                } else {
                    CYNG_LOG_ERROR(logger, "feed/adv output path [" << path << "] of report " << name << " does not exist");
                }
            } else {
                CYNG_LOG_TRACE(logger, "feed/adv report " << code << " (" << name << ") is disabled");
            }
        }
    }

} // namespace smf
