/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>

#include <influxdb.h>
#include <tasks/cleanup_db.h>
#include <tasks/csv_report.h>
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
#include <cyng/obj/set_cast.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/obj/vector_cast.hpp>
#include <cyng/parse/duration.h>
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

        std::locale loc(std::locale(), new std::ctype<char>);
        std::cout << std::locale("").name().c_str() << std::endl;

        return cyng::make_vector({cyng::make_tuple(
            cyng::make_param("generated", now),
            cyng::make_param("version", SMF_VERSION_TAG),
            cyng::make_param("tag", get_random_tag()),
            cyng::make_param("country.code", cyng::sys::get_system_locale().at(cyng::sys::info::COUNTRY)),
            cyng::make_param("language.code", cyng::sys::get_system_locale().at(cyng::sys::info::LANGUAGE)),
            // cyng::make_param("utc.offset", cyng::sys::delta_utc(now).count()),
            cyng::make_param("model", "smf.store"),                     //  ip-t ident
            cyng::make_param("network.delay", std::chrono::seconds(6)), //  seconds to wait before starting ip-t client

            //  list of database(s)
            cyng::make_param(
                "db",
                cyng::make_param(
                    "default",
                    cyng::tuple_factory(
                        cyng::make_param("connection.type", "SQLite"),
                        cyng::make_param("file.name", (cwd / "store.database").string()),
                        cyng::make_param("busy.timeout", std::chrono::milliseconds(12)), //	seconds
                        cyng::make_param("watchdog", std::chrono::seconds(30)),          //	for database connection
                        cyng::make_param("pool.size", 1),                                //	no pooling for SQLite
                        cyng::make_param("db.schema", SMF_VERSION_NAME), //	use "v4.0" for compatibility to version 4.x
                        cyng::make_param("interval", 12),                //	seconds
                        cyng::make_param(
                            "cleanup",
                            cyng::make_tuple(
                                create_cleanup_spec(OBIS_PROFILE_1_MINUTE, std::chrono::hours(48), true),
                                create_cleanup_spec(OBIS_PROFILE_15_MINUTE, std::chrono::hours(48), true),
                                create_cleanup_spec(OBIS_PROFILE_60_MINUTE, std::chrono::hours(800), true),
                                create_cleanup_spec(OBIS_PROFILE_24_HOUR, std::chrono::hours(800), true))), // cleanup
                        cyng::make_param(
                            "gap",
                            cyng::make_tuple(
                                create_gap_spec(OBIS_PROFILE_1_MINUTE, cwd, std::chrono::hours(48), false),
                                create_gap_spec(OBIS_PROFILE_15_MINUTE, cwd, std::chrono::hours(48), true),
                                create_gap_spec(OBIS_PROFILE_60_MINUTE, cwd, std::chrono::hours(800), true),
                                create_gap_spec(OBIS_PROFILE_24_HOUR, cwd, std::chrono::hours(800), true))) // gap
                        ))                                                                                  // default
                ),

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
                    cyng::make_param("prefix", "sml-protocol"),
                    cyng::make_param("suffix", "log"),
                    cyng::make_param("version", SMF_VERSION_NAME))),
            cyng::make_param(
                "IEC:LOG",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "log").string()),
                    cyng::make_param("prefix", "iec-protocol"),
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
                             "def.sk", "0102030405060708090001020304050607080900010203040506070809000001"), //	scramble key
                         cyng::make_param("scrambled", true)),
                     cyng::make_tuple(
                         cyng::make_param("host", "localhost"),
                         cyng::make_param("service", "26863"),
                         cyng::make_param("account", "data-store"),
                         cyng::make_param("pwd", "to-define"),
                         cyng::make_param(
                             "def.sk", "0102030405060708090001020304050607080900010203040506070809000001"), //	scramble key
                         cyng::make_param("scrambled", false))})),
            cyng::make_param(
                "targets",
                cyng::make_tuple(
                    cyng::make_param("SML", cyng::make_vector({"water@solostec", "gas@solostec", "power@solostec"})),
                    cyng::make_param("DLMS", cyng::make_vector({"dlms@store"})),
                    cyng::make_param("IEC", cyng::make_vector({"LZQJ", "iec@store"})))),
            cyng::make_param(
                "csv.reports",
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
                    ) // reports tuple
                ),    // reports param
            cyng::make_param(
                "lpex.reports",
                cyng::make_tuple(
                    cyng::make_param("db", "default"),
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
                    // only this obis codes will be accepted
                    cyng::make_param("filter", cyng::obis_path_t{OBIS_REG_POS_ACT_E, OBIS_REG_POS_ACT_E_T1, OBIS_REG_GAS_MC_0_0}),
                    create_lpex_spec(OBIS_PROFILE_15_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_15_MINUTE)),
                    create_lpex_spec(OBIS_PROFILE_60_MINUTE, cwd, false, sml::backtrack_time(OBIS_PROFILE_60_MINUTE)),
                    create_lpex_spec(OBIS_PROFILE_24_HOUR, cwd, false, sml::backtrack_time(OBIS_PROFILE_24_HOUR)),
                    create_lpex_spec(OBIS_PROFILE_1_MONTH, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MONTH))) // lpex tuple
                )                                                                                                  // lpex param
            )});
    }

    cyng::prop_t create_report_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack) {
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
                cyng::make_param("path", (cwd / "lpex.reports" / get_prefix(profile)).string()),
                cyng::make_param("backtrack", backtrack),
                cyng::make_param("prefix", ""),
                cyng::make_param("separated.by.devices", false), // individual reports for each device
                cyng::make_param("enabled", enabled)));
    }

    cyng::prop_t create_cleanup_spec(cyng::obis profile, std::chrono::hours hours, bool enabled) {
        return cyng::make_prop(
            profile,
            cyng::make_tuple(
                cyng::make_param("name", obis::get_name(profile)),
                cyng::make_param("max-age", hours),
                cyng::make_param("limit", 256),
                cyng::make_param("enabled", enabled)));
    }

    cyng::prop_t create_gap_spec(cyng::obis profile, std::filesystem::path const &cwd, std::chrono::hours hours, bool enabled) {
        return cyng::make_prop(
            profile,
            cyng::make_tuple(
                cyng::make_param("name", obis::get_name(profile)),
                cyng::make_param("path", (cwd / "gap-reports" / get_prefix(profile)).string()),
                cyng::make_param("backtrack", hours),
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
        auto const now = std::chrono::system_clock::now();
        // auto const utc_offset = cyng::sys::delta_utc(now);
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

        //
        // start csv reports
        //
        auto csv_reports = cyng::container_cast<cyng::param_map_t>(reader.get("csv.reports"));
        if (!csv_reports.empty()) {
            auto const db = reader["csv.reports"].get("db", "default");
            auto const pos = sm.find(db);
            if (pos != sm.end()) {
                start_csv_reports(ctl, channels, logger, pos->second, csv_reports);
            } else {
                CYNG_LOG_FATAL(logger, "no database [" << db << "] for CSV reports configured");
            }
        }
        //
        // start LPEx reports
        //
        auto lpex_reports = cyng::container_cast<cyng::param_map_t>(reader.get("lpex.reports"));
        if (!lpex_reports.empty()) {
            auto const db = reader["lpex.reports"].get("db", "default");
            auto const print_version = reader["lpex.reports"].get("print.version", true);
            auto const debug_mode = reader["lpex.reports"].get(
                "debug",
#ifdef _DEBUG
                true
#else
                false
#endif
            );
            auto const filter = cyng::to_obis_path(reader["lpex.reports"].get("filter", ""));
            auto const pos = sm.find(db);
            if (pos != sm.end()) {
                if (debug_mode) {
                    CYNG_LOG_WARNING(logger, "LPEx reports in debug mode");
                }
                start_lpex_reports(ctl, channels, logger, pos->second, lpex_reports, filter, print_version, debug_mode);
            } else {
                CYNG_LOG_FATAL(logger, "no database [" << db << "] for LPEx reports configured");
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
        // auto const delay = cyng::numeric_cast<std::uint32_t>(reader["network.delay"].get(), 6);
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
        for (auto const &tsk : cleanup_tasks) {
            auto const reader_cls = cyng::make_reader(tsk.second);

            auto const profile = cyng::to_obis(tsk.first);
            auto const enabled = reader_cls.get("enabled", false);
            if (enabled) {
                BOOST_ASSERT(sml::is_profile(profile));
                auto const name = reader_cls.get("name", "");
                auto const age = cyng::to_hours(reader_cls.get("max.age", "120:00:00"));
                auto const limit = reader_cls.get("limit", 256u);
                CYNG_LOG_INFO(
                    logger,
                    "start db cleanup task \"" << name << "\" for profile " << obis::get_name(profile) << " in "
                                               << (age.count() / 2) << " hours");
                auto channel = ctl.create_named_channel_with_ref<cleanup_db>("cleanup-db", ctl, logger, db, profile, age, limit);
                BOOST_ASSERT(channel->is_open());
                // don't start immediately
                channel->suspend(age / 2, "run", age / 4);

            } else {
                CYNG_LOG_WARNING(
                    logger, "db cleanup task " << name << " for profile " << obis::get_name(profile) << " is disabled");
            }
        }
    }

    void controller::start_gap_reports(
        cyng::controller &ctl,
        cyng::logger logger,
        std::string name,
        cyng::db::session db,
        cyng::param_map_t &&gap_tasks) {

        for (auto const &tsk : gap_tasks) {
            auto const reader_cls = cyng::make_reader(tsk.second);

            auto const profile = cyng::to_obis(tsk.first);
            auto const enabled = reader_cls.get("enabled", false);
            if (enabled) {
                BOOST_ASSERT(sml::is_profile(profile));
                CYNG_LOG_INFO(logger, "start gap report on db \"" << name << "\" for profile " << obis::get_name(profile));
                auto const name = reader_cls.get("name", "");
                auto const root = reader_cls.get("path", "");
                if (!std::filesystem::exists(root)) {
                    std::cout << "***warning: output path [" << root << "] of gap report " << name << " does not exists";
                    std::error_code ec;
                    if (!std::filesystem::create_directories(root, ec)) {
                        std::cerr << "***error: cannot create path [" << root << "]: " << ec.message();
                    }
                }

                auto const age = cyng::to_hours(reader_cls.get("max.age", "120:00:00"));
                auto channel = ctl.create_named_channel_with_ref<gap_report>("gap-report", ctl, logger, db, profile, root, age);
                BOOST_ASSERT(channel->is_open());
                channel->dispatch("run", age / 2);
            } else {
                CYNG_LOG_WARNING(
                    logger, "gap report on db " << name << " for profile " << obis::get_name(profile) << " is disabled");
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

        auto channel = ctl.create_named_channel_with_ref<network>(
            "network", ctl, tag, logger, node_name, model, std::move(tgl), sml_targets, iec_targets, dlms_targets, writer);
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
            if (boost::algorithm::equals(type, "csv")) {
                generate_csv_reports(read_config_section(config_.json_path_, config_.config_index_));
            } else if (boost::algorithm::equals(type, "lpex")) {
                generate_lpex_reports(read_config_section(config_.json_path_, config_.config_index_));
            } else if (boost::algorithm::equals(type, "gap")) {
                generate_gap_reports(read_config_section(config_.json_path_, config_.config_index_));
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

    void controller::generate_csv_reports(cyng::object &&cfg) {

        //
        //  data base connections
        //
        auto sm = init_storage(cfg, false);
        auto const reader = cyng::make_reader(std::move(cfg));
        auto const db = reader["csv.reports"].get("db", "default");

        auto const pos = sm.find(db);
        if (pos != sm.end()) {

            if (pos->second.is_alive()) {
                std::cout << "***info: file-name: " << reader["db"][db].get<std::string>("file.name", "") << std::endl;
                auto const cwd = std::filesystem::current_path();
                auto const now = std::chrono::system_clock::now();

                auto reports = cyng::container_cast<cyng::param_map_t>(reader.get("csv.reports"));
                for (auto const &cfg_report : reports) {

                    auto const reader_report = cyng::make_reader(cfg_report.second);
                    auto const name = reader_report.get("name", "no-name");

                    if (reader_report.get("enabled", false)) {
                        auto const profile = cyng::to_obis(cfg_report.first);
                        std::cout << "***info: generate csv report " << name << " (" << profile << ")" << std::endl;

                        auto const root = reader_report.get("path", cwd.string());
                        if (!std::filesystem::exists(root)) {
                            std::cout << "***warning: output path [" << root << "] of csv report " << name << " does not exists";
                            std::error_code ec;
                            if (!std::filesystem::create_directories(root, ec)) {
                                std::cerr << "***error: cannot create path [" << root << "]: " << ec.message();
                            }
                        }
                        auto const backtrack = cyng::to_hours(reader_report.get("backtrack", "40:00:00"));
                        auto const prefix = reader_report.get("prefix", "");
                        generate_csv(pos->second, profile, root, backtrack, now, prefix);

                    } else {
                        std::cout << "***info: csv report " << name << " is disabled" << std::endl;
                    }
                }
            }
        } else {
            std::cerr << "***error: csv report uses an undefined database: " << db << std::endl;
        }
    }

    void controller::generate_lpex_reports(cyng::object &&cfg) {

        //
        //  data base connections
        //
        auto sm = init_storage(cfg, false);
        auto const reader = cyng::make_reader(std::move(cfg));

        auto const print_version = reader["lpex.reports"].get("print.version", true);
        auto const debug_mode = reader["lpex.reports"].get(
            "debug",
#ifdef _DEBUG
            true
#else
            false
#endif
        );
        auto const filter = cyng::to_obis_path(reader["lpex.reports"].get("filter", ""));
        auto const db = reader["lpex.reports"].get("db", "default");
        auto const pos = sm.find(db);
        if (pos != sm.end()) {

            if (pos->second.is_alive()) {
                std::cout << "***info: file-name: " << reader["db"][db].get<std::string>("file.name", "") << std::endl;
                auto const cwd = std::filesystem::current_path();
                auto const now = cyng::make_utc_date();

                auto reports = cyng::container_cast<cyng::param_map_t>(reader.get("lpex.reports"));
                for (auto const &cfg_report : reports) {

                    if (!boost::algorithm::equals(cfg_report.first, "print.version") &&
                        !boost::algorithm::equals(cfg_report.first, "db") && !boost::algorithm::equals(cfg_report.first, "debug") &&
                        !boost::algorithm::starts_with(cfg_report.first, "filter") &&
                        !boost::algorithm::starts_with(cfg_report.first, "separated.by.devices")) {

                        auto const reader_report = cyng::make_reader(cfg_report.second);
                        auto const name = reader_report.get("name", "no-name");

                        if (reader_report.get("enabled", false)) {
                            auto const profile = cyng::to_obis(cfg_report.first);
                            std::cout << "***info: generate lpex report " << name << " (" << profile << ")" << std::endl;
                            auto const root = reader_report.get("path", cwd.string());
                            if (!std::filesystem::exists(root)) {
                                std::cout << "***warning: output path [" << root << "] of lpex report " << name
                                          << " does not exists";
                                std::error_code ec;
                                if (!std::filesystem::create_directories(root, ec)) {
                                    std::cerr << "***error: cannot create path [" << root << "]: " << ec.message();
                                }
                            }
                            auto const backtrack = cyng::to_hours(reader_report.get("backtrack", "40:00:00"));
                            auto const separated = reader_report.get("separated.by.devices", false);

                            auto const prefix = reader_report.get("prefix", "LPEx-");
                            generate_lpex(
                                pos->second, profile, filter, root, now, backtrack, prefix, print_version, separated, debug_mode);

                        } else {
                            std::cout << "***info: lpex report " << name << " is disabled" << std::endl;
                        }
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

    void controller::cleanup_archive(cyng::object &&cfg) {
        auto const now = std::chrono::system_clock::now();
        auto const reader = cyng::make_reader(cfg);
        BOOST_ASSERT(reader.get("db").tag() == cyng::TC_PARAM_MAP);
        auto const pm = cyng::container_cast<cyng::param_map_t>(reader.get("db"));
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
                    if (enabled) {
                        auto const age = cyng::to_hours(reader_cls.get("max.age", "48:00:00"));

                        auto const d = cyng::date::make_date_from_local_time(now - age);
                        std::cout << "start cleanup task on db \"" << param.first << "\" for profile " << obis::get_name(profile)
                                  << " older than " << cyng::as_string(d, "%Y-%m-%d %H:%M") << std::endl;
                        auto const limit = reader_cls.get("limit", 256u);
                        auto const size = smf::cleanup(s, profile, now - age, limit);
                        if (size != 0) {
                            std::cout << size << " records removed" << std::endl;
                        }
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
            auto channel = ctl.create_named_channel_with_ref<sml_log_writer>(name, ctl, logger, out, prefix, suffix);
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

    void controller::start_csv_reports(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::db::session db,
        cyng::param_map_t reports) {

        // "utc.offset"
        //
        //	start reporting
        //
        for (auto const &cfg : reports) {
            auto const reader = cyng::make_reader(cfg.second);
            if (reader.get("enabled", false)) {

                auto const profile = cyng::to_obis(cfg.first);
                BOOST_ASSERT(sml::is_profile(profile));
                auto const name = reader.get("name", "");
                auto const path = reader.get("path", "");
                auto const backtrack = cyng::to_hours(reader.get("backtrack", "10:00:00"));
                auto const prefix = reader.get("prefix", "");

                if (!std::filesystem::exists(path)) {
                    CYNG_LOG_WARNING(logger, "output path [" << path << "] of CSV report " << name << " does not exists");
                    std::error_code ec;
                    if (!std::filesystem::create_directories(path, ec)) {
                        CYNG_LOG_ERROR(logger, "cannot create path [" << path << "]: " << ec.message());
                    }
                }

                auto channel =
                    ctl.create_named_channel_with_ref<csv_report>(name, ctl, logger, db, profile, path, backtrack, prefix);
                BOOST_ASSERT(channel->is_open());
                channels.lock(channel);

                //
                //  calculate start time
                //
                auto const now = std::chrono::system_clock::now();
                auto const next = sml::floor(now + sml::interval_time(now, profile), profile);
                CYNG_LOG_INFO(logger, "start CSV report " << profile << " (" << name << ") at " << next);
                // bus_.sys_msg(cyng::severity::LEVEL_INFO, "start report ", profile, " (", name, ") at ", next, " UTC");

                if (next > now) {
                    channel->suspend(next - now, "run");
                } else {
                    channel->suspend(sml::interval_time(now, profile), "run");
                }

            } else {
                CYNG_LOG_TRACE(logger, "CSV report " << cfg.first << " is disabled");
            }
        }
    }

    void controller::start_lpex_reports(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::db::session db,
        cyng::param_map_t reports,
        cyng::obis_path_t filter,
        bool print_version,
        bool debug_mode) {

        //
        //	start LPEx reporting
        //
        for (auto const &cfg : reports) {
            //
            //  skip "print.version" and "db" entry
            //
            if (!boost::algorithm::equals(cfg.first, "print.version") && !boost::algorithm::equals(cfg.first, "db") &&
                !boost::algorithm::equals(cfg.first, "debug") && !boost::algorithm::starts_with(cfg.first, "filter")) {
                auto const reader = cyng::make_reader(cfg.second);
                auto const name = reader.get("name", "");
                if (reader.get("enabled", false)) {

                    auto const profile = cyng::to_obis(cfg.first);
                    BOOST_ASSERT(sml::is_profile(profile));
                    auto const path = reader.get("path", "");
                    auto const backtrack = cyng::to_hours(reader.get("backtrack", "10:00:00"));
                    auto const prefix = reader.get("prefix", "");
                    auto const separated = reader.get("separated.by.devices", false);

                    if (!std::filesystem::exists(path)) {
                        CYNG_LOG_WARNING(logger, "output path [" << path << "] of LPEx report " << name << " does not exists");
                        std::error_code ec;
                        if (!std::filesystem::create_directories(path, ec)) {
                            CYNG_LOG_ERROR(logger, "cannot create path [" << path << "]: " << ec.message());
                        }
                    }

                    auto channel = ctl.create_named_channel_with_ref<lpex_report>(
                        name, ctl, logger, db, profile, filter, path, backtrack, prefix, print_version, separated, debug_mode);
                    BOOST_ASSERT(channel->is_open());
                    channels.lock(channel);

                    //
                    //  calculate start time
                    //
                    auto const now = std::chrono::system_clock::now();
                    auto const interval = sml::interval_time(now, profile);
                    auto const next = sml::floor(now + interval, profile);

                    if (next > now) {
                        auto const d = cyng::date::make_date_from_local_time(next);
                        CYNG_LOG_INFO(
                            logger, "start LPEx report " << profile << " (" << name << ") at " << cyng::as_string(d, "%F %T%z"));
                        channel->suspend(next - now, "run");
                    } else {
                        auto const d = cyng::date::make_date_from_local_time(now + interval);
                        CYNG_LOG_INFO(
                            logger, "start LPEx report " << profile << " (" << name << ") at " << cyng::as_string(d, "%F %T%z"));
                        channel->suspend(interval, "run");
                    }
                } else {
                    CYNG_LOG_TRACE(logger, "LPEx report " << cfg.first << " (" << name << ") is disabled");
                }
            }
        }
    }
} // namespace smf
