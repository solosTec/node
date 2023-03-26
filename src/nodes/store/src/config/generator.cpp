#include <config/generator.h>

#include <smf/report/config/gen_csv.h>
#include <smf/report/config/gen_feed.h>
#include <smf/report/config/gen_gap.h>
#include <smf/report/config/gen_lpex.h>

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
                            cfg::gen_gap(cwd)) // gap
                        ))),

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
                cfg::gen_csv(cwd)), // "csv.reports"
            cyng::make_param(
                "lpex.reports",
                cfg::gen_lpex(cwd)), // "lpex.reports"
            cyng::make_param(
                "feed.reports",
                cfg::gen_feed(cwd)) // "feed.reports"
            )});
    }

    cyng::prop_t create_cleanup_spec(cyng::obis profile, std::chrono::hours hours, bool enabled) {
        return cyng::make_prop(
            profile,
            cyng::make_tuple(
                cyng::make_param("name", obis::get_name(profile)),
                cyng::make_param("max.age", hours),
                cyng::make_param("enabled", enabled)));
    }

} // namespace smf
