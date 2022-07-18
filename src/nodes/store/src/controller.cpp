/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>

#include <influxdb.h>
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
#include <cyng/obj/set_cast.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/obj/vector_cast.hpp>
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

        std::locale loc(std::locale(), new std::ctype<char>);
        std::cout << std::locale("").name().c_str() << std::endl;

        return cyng::make_vector({cyng::make_tuple(
            cyng::make_param("generated", now),
            cyng::make_param("version", SMF_VERSION_TAG),
            cyng::make_param("log-dir", tmp.string()),
            cyng::make_param("tag", get_random_tag()),
            cyng::make_param("country-code", cyng::sys::get_system_locale().at(cyng::sys::info::COUNTRY)),
            cyng::make_param("language-code", cyng::sys::get_system_locale().at(cyng::sys::info::LANGUAGE)),
            cyng::make_param("model", "smf.store"), //  ip-t ident
            cyng::make_param("network-delay", 6),   //  seconds to wait before starting ip-t client

            //  list of database(s)
            cyng::make_param(
                "db",
                cyng::make_param(
                    "default",
                    cyng::tuple_factory(
                        cyng::make_param("connection-type", "SQLite"),
                        cyng::make_param("file-name", (cwd / "store.database").string()),
                        cyng::make_param("busy-timeout", 12),            //	seconds
                        cyng::make_param("watchdog", 30),                //	for database connection
                        cyng::make_param("pool-size", 1),                //	no pooling for SQLite
                        cyng::make_param("db-schema", SMF_VERSION_NAME), //	use "v4.0" for compatibility to version 4.x
                        cyng::make_param("interval", 12)                 //	seconds
                        ))),

            cyng::make_param("writer", cyng::make_vector({"ALL:BIN"})), //	options are XML, JSON, DB, BIN, ...

            cyng::make_param("SML:DB", cyng::tuple_factory(cyng::make_param("db", "default"))),
            cyng::make_param("IEC:DB", cyng::tuple_factory(cyng::make_param("db", "default"))),
            cyng::make_param(
                "SML:XML",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "xml").string()),
                    cyng::make_param("root-name", "SML"),
                    cyng::make_param("endcoding", "UTF-8"))),
            cyng::make_param(
                "SML:JSON",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "json").string()),
                    cyng::make_param("prefix", "sml-"),
                    cyng::make_param("suffix", "json"),
                    cyng::make_param("version", SMF_VERSION_NAME))),
            cyng::make_param(
                "SML:ABL",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "abl").string()),
                    cyng::make_param("prefix", "sml-"),
                    cyng::make_param("suffix", "abl"),
                    cyng::make_param("version", SMF_VERSION_NAME),
                    cyng::make_param("line-ending", "DOS") //	DOS/UNIX, DOS = "\r\n", UNIX = "\n", native = std::endl
                    )),
            cyng::make_param(
                "ALL:BIN",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "sml").string()),
                    cyng::make_param("prefix", "smf-"),
                    cyng::make_param("suffix", "bin"),
                    cyng::make_param("version", SMF_VERSION_NAME))),
            cyng::make_param(
                "SML:LOG",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "log").string()),
                    cyng::make_param("prefix", "sml-"),
                    cyng::make_param("suffix", "log"),
                    cyng::make_param("version", SMF_VERSION_NAME))),
            cyng::make_param(
                "IEC:LOG",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "log").string()),
                    cyng::make_param("prefix", "iec-"),
                    cyng::make_param("suffix", "log"),
                    cyng::make_param("version", SMF_VERSION_NAME))),
            cyng::make_param(
                "SML:CSV",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "csv").string()),
                    cyng::make_param("prefix", "sml-"),
                    cyng::make_param("suffix", "csv"),
                    cyng::make_param("header", true),
                    cyng::make_param("version", SMF_VERSION_NAME))),
            cyng::make_param(
                "IEC:CSV",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "csv").string()),
                    cyng::make_param("prefix", "iec-"),
                    cyng::make_param("suffix", "csv"),
                    cyng::make_param("header", true),
                    cyng::make_param("version", SMF_VERSION_NAME))),
            cyng::make_param(
                "SML:influxdb",
                cyng::tuple_factory(
                    cyng::make_param("host", "localhost"),
                    cyng::make_param("port", "8086"),     //	8094 for udp
                    cyng::make_param("protocol", "http"), //	http, https, udp, unix
                    cyng::make_param("cert", (cwd / "cert.pem").string()),
                    cyng::make_param("db", "SMF"),
                    cyng::make_param("series", "SML"))),
            cyng::make_param(
                "IEC:influxdb",
                cyng::tuple_factory(
                    cyng::make_param("host", "localhost"),
                    cyng::make_param("port", "8086"),     //	8094 for udp
                    cyng::make_param("protocol", "http"), //	http, https, udp, unix
                    cyng::make_param("cert", (cwd / "cert.pem").string()),
                    cyng::make_param("db", "SMF"),
                    cyng::make_param("series", "IEC"))),
            cyng::make_param(
                "DLMS:influxdb",
                cyng::tuple_factory(
                    cyng::make_param("host", "localhost"),
                    cyng::make_param("port", "8086"),     //	8094 for udp
                    cyng::make_param("protocol", "http"), //	http, https, udp, unix
                    cyng::make_param("cert", (cwd / "cert.pem").string()),
                    cyng::make_param("db", "SMF"),
                    cyng::make_param("series", "DLMS"))),
            cyng::make_param(
                "ipt",
                cyng::make_vector(
                    {cyng::make_tuple(
                         cyng::make_param("host", "localhost"),
                         cyng::make_param("service", "26862"),
                         cyng::make_param("account", "data-store"),
                         cyng::make_param("pwd", "to-define"),
                         cyng::make_param(
                             "def-sk", "0102030405060708090001020304050607080900010203040506070809000001"), //	scramble key
                         cyng::make_param("scrambled", true)),
                     cyng::make_tuple(
                         cyng::make_param("host", "localhost"),
                         cyng::make_param("service", "26863"),
                         cyng::make_param("account", "data-store"),
                         cyng::make_param("pwd", "to-define"),
                         cyng::make_param(
                             "def-sk", "0102030405060708090001020304050607080900010203040506070809000001"), //	scramble key
                         cyng::make_param("scrambled", false))})),
            cyng::make_param(
                "targets",
                cyng::make_tuple(
                    cyng::make_param("SML", cyng::make_vector({"water@solostec", "gas@solostec", "power@solostec"})),
                    cyng::make_param("DLMS", cyng::make_vector({"dlms@store"})),
                    cyng::make_param("IEC", cyng::make_vector({"LZQJ", "iec@store"})))),
            cyng::make_param(
                "reports",
                cyng::make_tuple(
                    cyng::make_param("db", "default"),
                    create_report_spec(OBIS_PROFILE_1_MINUTE, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MINUTE)),
                    create_report_spec(OBIS_PROFILE_15_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_15_MINUTE)),
                    create_report_spec(OBIS_PROFILE_60_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_60_MINUTE)),
                    create_report_spec(OBIS_PROFILE_24_HOUR, cwd, true, sml::backtrack_time(OBIS_PROFILE_24_HOUR)),
                    // create_report_spec(OBIS_PROFILE_LAST_2_HOURS, cwd, false, sml::backtrack_time(OBIS_PROFILE_LAST_2_HOURS)),
                    // create_report_spec(OBIS_PROFILE_LAST_WEEK, cwd, false, sml::backtrack_time(OBIS_PROFILE_LAST_WEEK)),
                    create_report_spec(OBIS_PROFILE_1_MONTH, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MONTH)), // one month
                    create_report_spec(OBIS_PROFILE_1_YEAR, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_YEAR))    //  one year
                    )                                                                                                // reports
                ))});
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

        auto const reader = cyng::make_reader(cfg);
        auto const tag = read_tag(reader["tag"].get());
        auto const model = cyng::value_cast(reader["model"].get(), "smf.store");

        auto const ipt_vec = cyng::container_cast<cyng::vector_t>(reader["ipt"].get());
        auto tgl = ipt::read_config(ipt_vec);
        if (tgl.empty()) {
            CYNG_LOG_FATAL(logger, "no ip-t server configured");
        }

        //
        //  data base connections
        //
        auto sm = init_storage(cfg);

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
                        start_sml_db(
                            ctl, channels, logger, pos->second, cyng::container_cast<cyng::param_map_t>(reader.get(name)), name);
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
        auto const delay = cyng::numeric_cast<std::uint32_t>(reader["network-delay"].get(), 6);
        CYNG_LOG_INFO(logger, "start ipt bus in " << delay << " seconds");

        //
        //  connect to ip-t server
        //
        join_network(
            ctl,
            channels,
            logger,
            tag,
            node_name,
            model,
            std::move(tgl),
            std::chrono::seconds(delay),
            target_sml,
            target_iec,
            target_dlms,
            writer);
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

        auto channel = ctl.create_named_channel_with_ref<network>(
            "network",
            ctl,
            tag,
            logger,
            node_name,
            model,
            std::move(tgl),
            // config_types,
            sml_targets,
            iec_targets,
            dlms_targets,
            writer);
        BOOST_ASSERT(channel->is_open());
        channels.lock(channel);

        //
        //  wait for IP-T server to start up
        //
        channel->suspend(delay, "connect");
    }

    bool controller::run_options(boost::program_options::variables_map &vars) {

        if (vars["init"].as<bool>()) {
            //	initialize database

            //
            //  read configuration
            //
            auto const cfg = read_config_section(config_.json_path_, config_.config_index_);
            auto sm = init_storage(cfg);
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
                        std::cerr << "***error: writer " << name << " uses an undefined database" << std::endl;
                    }
                } else if (boost::algorithm::equals(name, "IEC:DB")) {
                    auto const db = reader[name].get("db", "default");
                    auto const pos = sm.find(db);
                    if (pos != sm.end()) {
                        std::cout << "***info : writer " << name << " initialize database " << pos->first << std::endl;
                        iec_db_writer::init_storage(pos->second);
                    } else {
                        std::cerr << "***error: writer " << name << " uses an undefined database" << std::endl;
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
        //  generate different reports
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

            std::cout << "***info: generate 24 h reports: " << std::endl;
            generate_report(s, OBIS_PROFILE_24_HOUR, cwd, std::chrono::hours(40), now);
        }
    }
    std::map<std::string, cyng::db::session> controller::init_storage(cyng::object const &cfg) {

        std::map<std::string, cyng::db::session> sm;
        auto const reader = cyng::make_reader(cfg);
        BOOST_ASSERT(reader.get("db").tag() == cyng::TC_PARAM_MAP);
        auto const pm = cyng::container_cast<cyng::param_map_t>(reader.get("db"));
        for (auto const &param : pm) {
            std::cout << "create database [" << param.first << "]" << std::endl;
            auto const db = cyng::container_cast<cyng::param_map_t>(param.second);
            sm.emplace(param.first, init_storage(db));
        }
        return sm;
    }

    cyng::db::session controller::init_storage(cyng::param_map_t const &pm) {
        auto s = cyng::db::create_db_session(pm);
        if (s.is_alive()) {
            std::cout << "***info : database created" << std::endl;
            // smf::init_storage(s);	//	see task/storage_db.h
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
        cyng::param_map_t &&pm,
        std::string const &name) {
        if (!pm.empty()) {
            CYNG_LOG_INFO(logger, "start sml database writer");
            auto const reader = cyng::make_reader(pm);
            auto channel = ctl.create_named_channel_with_ref<sml_db_writer>(name, ctl, logger, db);
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
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
            auto channel = ctl.create_named_channel_with_ref<sml_xml_writer>(name, ctl, logger, out, prefix, suffix);
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
            auto channel = ctl.create_named_channel_with_ref<sml_json_writer>(name, ctl, logger, out, prefix, suffix);
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
        if (!out.empty()) {
            auto channel = ctl.create_named_channel_with_ref<sml_abl_writer>(name, ctl, logger, out, prefix, suffix, eol_dos);
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
        CYNG_LOG_INFO(logger, "start sml log writer -> " << out);
        if (!out.empty()) {
            auto channel = ctl.create_named_channel_with_ref<sml_log_writer>(name, ctl, logger);
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

        if (!out.empty()) {

            auto channel = ctl.create_named_channel_with_ref<sml_csv_writer>(name, ctl, logger, out, prefix, suffix, header);
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
        auto channel = ctl.create_named_channel_with_ref<sml_influx_writer>(name, ctl, logger, host, service, protocol, cert, db);
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
            auto channel = ctl.create_named_channel_with_ref<iec_db_writer>(name, ctl, logger, db);
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
            auto channel = ctl.create_named_channel_with_ref<iec_json_writer>(name, ctl, logger, out, prefix, suffix);
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
            auto channel = ctl.create_named_channel_with_ref<iec_log_writer>(name, ctl, logger, out, prefix, suffix);
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
            auto channel = ctl.create_named_channel_with_ref<iec_csv_writer>(name, ctl, logger, out, prefix, suffix, header);
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
        auto channel = ctl.create_named_channel_with_ref<iec_influx_writer>(name, ctl, logger, host, service, protocol, cert, db);
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
        auto channel = ctl.create_named_channel_with_ref<dlms_influx_writer>(name, ctl, logger, host, service, protocol, cert, db);
        BOOST_ASSERT(channel->is_open());
        channels.lock(channel);
    }

} // namespace smf
