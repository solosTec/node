#include <config/generator.h>

#include <smf.h>
#include <smf/obis/db.h>
#include <smf/obis/defs.h>
#include <smf/obis/profile.h>

#include <cyng/obj/container_factory.hpp>

#ifdef _DEBUG_SEGW
#include <iostream>
#endif

namespace smf {
    cyng::vector_t create_default_config(
        std::chrono::system_clock::time_point &&now,
        std::filesystem::path &&tmp,
        std::filesystem::path &&cwd,
        boost::uuids::uuid tag) {
        return cyng::make_vector({cyng::make_tuple(
            cyng::make_param("generated", now),
            cyng::make_param("version", SMF_VERSION_TAG),
            cyng::make_param("tag", tag),
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

            cyng::make_param(
                "SML:DB",
                cyng::tuple_factory(
                    cyng::make_param("db", "default"),
                    cyng::make_param(
                        "exclude.profiles", cyng::obis_path_t{OBIS_PROFILE_1_MINUTE, OBIS_PROFILE_1_YEAR, OBIS_PROFILE_INITIAL}))),
            cyng::make_param(
                "IEC:DB",
                cyng::tuple_factory(
                    cyng::make_param("db", "default"),
                    cyng::make_param(
                        "exclude.profiles", cyng::obis_path_t{OBIS_PROFILE_1_MINUTE, OBIS_PROFILE_1_YEAR, OBIS_PROFILE_INITIAL}))),
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
                    cyng::make_param(
                        "profiles",
                        cyng::make_tuple(
                            create_report_spec(OBIS_PROFILE_1_MINUTE, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MINUTE)),
                            create_report_spec(OBIS_PROFILE_15_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_15_MINUTE)),
                            create_report_spec(OBIS_PROFILE_60_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_60_MINUTE)),
                            create_report_spec(OBIS_PROFILE_24_HOUR, cwd, true, sml::backtrack_time(OBIS_PROFILE_24_HOUR)),
                            // create_report_spec(OBIS_PROFILE_LAST_2_HOURS, cwd, false,
                            // sml::backtrack_time(OBIS_PROFILE_LAST_2_HOURS)), create_report_spec(OBIS_PROFILE_LAST_WEEK, cwd,
                            // false, sml::backtrack_time(OBIS_PROFILE_LAST_WEEK)),
                            create_report_spec(
                                OBIS_PROFILE_1_MONTH, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MONTH)), // one month
                            create_report_spec(
                                OBIS_PROFILE_1_YEAR, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_YEAR)) //  one year
                            )                                                                              // reports tuple
                        ))),                                                                               // reports param
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
                    cyng::make_param(
                        "profiles",
                        cyng::make_tuple(
                            create_lpex_spec(OBIS_PROFILE_15_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_15_MINUTE)),
                            create_lpex_spec(OBIS_PROFILE_60_MINUTE, cwd, false, sml::backtrack_time(OBIS_PROFILE_60_MINUTE)),
                            create_lpex_spec(OBIS_PROFILE_24_HOUR, cwd, false, sml::backtrack_time(OBIS_PROFILE_24_HOUR)),
                            create_lpex_spec(
                                OBIS_PROFILE_1_MONTH, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MONTH))) // lpex tuple
                        )                                                                                     // lpex param
                    )),                                                                                       // "lpex.reports"
            //
            cyng::make_param(
                "feed.reports",
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
                    // only this obis codes will be accepted
                    cyng::make_param(
                        "filter",
                        cyng::obis_path_t{
                            OBIS_REG_POS_ACT_E,
                            OBIS_REG_NEG_ACT_E,
                            OBIS_REG_HEAT_CURRENT,
                            OBIS_WATER_CURRENT,
                            OBIS_REG_GAS_MC_0_0,
                            OBIS_REG_GAS_MC_1_0}),
                    cyng::make_param(
                        "profiles",
                        cyng::make_tuple(
                            create_feed_spec(OBIS_PROFILE_15_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_15_MINUTE)),
                            create_feed_spec(OBIS_PROFILE_60_MINUTE, cwd, true, sml::backtrack_time(OBIS_PROFILE_60_MINUTE)),
                            create_feed_spec(OBIS_PROFILE_24_HOUR, cwd, false, sml::backtrack_time(OBIS_PROFILE_24_HOUR)),
                            create_feed_spec(
                                OBIS_PROFILE_1_MONTH, cwd, false, sml::backtrack_time(OBIS_PROFILE_1_MONTH))) // feed tuple
                        )                                                                                     // feed param
                    ))                                                                                        // "feed.reports"
                                                                                                              //
            )});
    }

    cyng::prop_t create_report_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack) {
        return cyng::make_prop(
            profile,
            cyng::make_tuple(
                cyng::make_param("name", obis::get_name(profile)),
                cyng::make_param("path", (cwd / "csv.reports" / sml::get_prefix(profile)).string()),
                cyng::make_param("backtrack", backtrack),
                cyng::make_param("prefix", ""),
                cyng::make_param("enabled", enabled)));
    }

    cyng::prop_t create_lpex_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack) {
        return cyng::make_prop(
            profile,
            cyng::make_tuple(
                cyng::make_param("name", obis::get_name(profile)),
                cyng::make_param("path", (cwd / "lpex.reports" / sml::get_prefix(profile)).string()),
                cyng::make_param("backtrack", backtrack),
                cyng::make_param("prefix", ""),
                cyng::make_param("add.customer.data", false), // add/update customer data
                cyng::make_param("enabled", enabled)));
    }

    cyng::prop_t create_feed_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack) {
        return cyng::make_prop(
            profile,
            cyng::make_tuple(
                cyng::make_param("name", obis::get_name(profile)),
                cyng::make_param("path", (cwd / "feed.reports" / sml::get_prefix(profile)).string()),
                cyng::make_param("backtrack", backtrack),
                cyng::make_param("prefix", ""),
                cyng::make_param("add.customer.data", false), // add/update customer data
                cyng::make_param("enabled", enabled)));
    }

    cyng::prop_t create_cleanup_spec(cyng::obis profile, std::chrono::hours hours, bool enabled) {
        return cyng::make_prop(
            profile,
            cyng::make_tuple(
                cyng::make_param("name", obis::get_name(profile)),
                cyng::make_param("max.age", hours),
                cyng::make_param("enabled", enabled)));
    }

    cyng::prop_t create_gap_spec(cyng::obis profile, std::filesystem::path const &cwd, std::chrono::hours hours, bool enabled) {
        return cyng::make_prop(
            profile,
            cyng::make_tuple(
                cyng::make_param("name", obis::get_name(profile)),
                cyng::make_param("path", (cwd / "gap-reports" / sml::get_prefix(profile)).string()),
                cyng::make_param("backtrack", hours),
                cyng::make_param("enabled", enabled)));
    }

} // namespace smf
