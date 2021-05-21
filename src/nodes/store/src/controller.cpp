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
#include <tasks/iec_log_writer.h>
#include <tasks/sml_abl_writer.h>
#include <tasks/sml_csv_writer.h>
#include <tasks/sml_db_writer.h>
#include <tasks/sml_influx_writer.h>
#include <tasks/sml_json_writer.h>
#include <tasks/sml_log_writer.h>
#include <tasks/sml_xml_writer.h>

#include <smf.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/object.h>
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
            cyng::make_param("country-code", "CH"),
            cyng::make_param("language-code", cyng::sys::get_system_locale()),
            cyng::make_param("model", "smf.store"),
            cyng::make_param("network-delay", 6), //  seconds to wait before starting ip-t client

            cyng::make_param("writer", cyng::make_vector({"ALL:BIN"})), //	options are XML, JSON, DB, BIN, ...

            cyng::make_param(
                "SML:DB",
                cyng::tuple_factory(
                    cyng::make_param("type", "SQLite"),
                    cyng::make_param("file-name", (cwd / "store.database").string()),
                    cyng::make_param("busy-timeout", 12),            //	seconds
                    cyng::make_param("watchdog", 30),                //	for database connection
                    cyng::make_param("pool-size", 1),                //	no pooling for SQLite
                    cyng::make_param("db-schema", SMF_VERSION_NAME), //	use "v4.0" for compatibility to version 4.x
                    cyng::make_param("interval", 12)                 //	seconds
                    )),
            cyng::make_param(
                "IEC:DB",
                cyng::tuple_factory(
                    cyng::make_param("type", "SQLite"),
                    cyng::make_param("file-name", (cwd / "store.database").string()),
                    cyng::make_param("busy-timeout", 12), //	seconds
                    cyng::make_param("watchdog", 30),     //	for database connection
                    cyng::make_param("pool-size", 1),     //	no pooling for SQLite
                    cyng::make_param("db-schema", SMF_VERSION_NAME),
                    // cyng::make_param("interval", 20),      //	seconds
                    cyng::make_param("ignore-null", false) //	don't write values equal 0
                    )),
            cyng::make_param(
                "SML:XML",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "xml").string()),
                    cyng::make_param("root-name", "SML"),
                    cyng::make_param("endcoding", "UTF-8"),
                    cyng::make_param("interval", 18) //	seconds
                    )),
            cyng::make_param(
                "SML:JSON",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "json").string()),
                    cyng::make_param("prefix", "smf"),
                    cyng::make_param("suffix", "json"),
                    cyng::make_param("version", SMF_VERSION_NAME),
                    cyng::make_param("interval", 21) //	seconds
                    )),
            cyng::make_param(
                "SML:ABL",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "abl").string()),
                    cyng::make_param("prefix", "smf"),
                    cyng::make_param("suffix", "abl"),
                    cyng::make_param("version", SMF_VERSION_NAME),
                    cyng::make_param("interval", 33),      //	seconds
                    cyng::make_param("line-ending", "DOS") //	DOS/UNIX
                    )),
            cyng::make_param(
                "ALL:BIN",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "sml").string()),
                    cyng::make_param("prefix", "smf"),
                    cyng::make_param("suffix", "sml"),
                    cyng::make_param("version", SMF_VERSION_NAME))),
            cyng::make_param(
                "SML:LOG",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "log").string()),
                    cyng::make_param("prefix", "sml"),
                    cyng::make_param("suffix", "log"),
                    cyng::make_param("version", SMF_VERSION_NAME))),
            cyng::make_param(
                "IEC:LOG",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "log").string()),
                    cyng::make_param("prefix", "iec"),
                    cyng::make_param("suffix", "log"),
                    cyng::make_param("version", SMF_VERSION_NAME))),
            cyng::make_param(
                "SML:CSV",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "csv").string()),
                    cyng::make_param("prefix", "sml"),
                    cyng::make_param("suffix", "csv"),
                    cyng::make_param("header", true),
                    cyng::make_param("version", SMF_VERSION_NAME))),
            cyng::make_param(
                "IEC:CSV",
                cyng::tuple_factory(
                    cyng::make_param("root-dir", (cwd / "csv").string()),
                    cyng::make_param("prefix", "iec"),
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
                    cyng::make_param("IEC", cyng::make_vector({"LZQJ", "iec@store"})))))});
    }
    void controller::run(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::object const &cfg,
        std::string const &node_name) {

        auto const reader = cyng::make_reader(cfg);
        auto const tag = read_tag(reader["tag"].get());
        auto const config_types = cyng::vector_cast<std::string>(reader["output"].get(), "ALL:BIN");
        auto const model = cyng::value_cast(reader["model"].get(), "smf.store");

        auto const ipt_vec = cyng::container_cast<cyng::vector_t>(reader["ipt"].get());
        auto tgl = ipt::read_config(ipt_vec);
        if (tgl.empty()) {
            CYNG_LOG_FATAL(logger, "no ip-t server configured");
        }

        //
        //  start writer tasks
        //
        auto const writer = cyng::vector_cast<std::string>(reader["writer"].get(), "ALL:BIN");
        if (!writer.empty()) {
            for (auto const &name : writer) {
                if (boost::algorithm::equals(name, "SML:DB")) {
                    start_sml_db(ctl, channels, logger, cyng::container_cast<cyng::param_map_t>(reader.get(name)), name);
                } else if (boost::algorithm::equals(name, "SML:SML")) {
                    start_sml_xml(ctl, channels, logger, cyng::container_cast<cyng::param_map_t>(reader.get(name)), name);
                } else if (boost::algorithm::equals(name, "SML:JSON")) {
                    start_sml_json(ctl, channels, logger, cyng::container_cast<cyng::param_map_t>(reader.get(name)), name);
                } else if (boost::algorithm::equals(name, "SML:ABL")) {
                    start_sml_abl(ctl, channels, logger, cyng::container_cast<cyng::param_map_t>(reader.get(name)), name);
                } else if (boost::algorithm::equals(name, "SML:LOG")) {
                    start_sml_log(ctl, channels, logger, cyng::container_cast<cyng::param_map_t>(reader.get(name)), name);
                } else if (boost::algorithm::equals(name, "SML:CSV")) {
                    start_sml_csv(ctl, channels, logger, cyng::container_cast<cyng::param_map_t>(reader.get(name)), name);
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
                    start_iec_db(ctl, channels, logger, cyng::container_cast<cyng::param_map_t>(reader.get(name)), name);
                } else if (boost::algorithm::equals(name, "IEC:LOG")) {
                    start_iec_log(ctl, channels, logger, cyng::container_cast<cyng::param_map_t>(reader.get(name)), name);
                } else if (boost::algorithm::equals(name, "IEC:CSV")) {
                    start_iec_csv(ctl, channels, logger, cyng::container_cast<cyng::param_map_t>(reader.get(name)), name);
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

        auto const target_sml = cyng::vector_cast<std::string>(reader["targets"]["SML"].get(), "sml@store");
        auto const target_iec = cyng::vector_cast<std::string>(reader["targets"]["IEC"].get(), "iec@store");
        auto const target_dlms = cyng::vector_cast<std::string>(reader["targets"]["DLMS"].get(), "dlms@store");

        if (target_sml.empty()) {
            CYNG_LOG_WARNING(logger, "no SML targets configured");
        }
        if (target_iec.empty()) {
            CYNG_LOG_WARNING(logger, "no IEC targets configured");
        }
        if (target_dlms.empty()) {
            CYNG_LOG_WARNING(logger, "no DLMS targets configured");
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
            config_types,
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
        std::vector<std::string> const &config_types,
        std::vector<std::string> const &sml_targets,
        std::vector<std::string> const &iec_targets,
        std::vector<std::string> const &dlms_targets,
        std::vector<std::string> const &writer) {

        auto channel = ctl.create_named_channel_with_ref<network>(
            "network",
            ctl,
            tag,
            logger,
            node_name,
            model,
            std::move(tgl),
            config_types,
            sml_targets,
            iec_targets,
            dlms_targets,
            writer);
        BOOST_ASSERT(channel->is_open());
        channels.lock(channel);

        //
        //  wait for IP-T server to start up
        //
        channel->suspend(delay, "connect", cyng::make_tuple());
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
        auto const cmd = vars["influxdb"].as<std::string>();
        if (!cmd.empty()) {
            //	execute a influx db command
            // std::cerr << cmd << " not implemented yet" << std::endl;
            // return ctrl.create_influx_dbs(config_index, cmd);
            create_influx_dbs(read_config_section(config_.json_path_, config_.config_index_), cmd);
            return true;
        }
        //

        //
        //	call base classe
        //
        return controller_base::run_options(vars);
    }

    void controller::init_storage(cyng::object &&cfg) {

        auto const reader = cyng::make_reader(std::move(cfg));
        // auto s = cyng::db::create_db_session(reader.get("DB"));
        // if (s.is_alive())	smf::init_storage(s);	//	see task/storage_db.h
    }

    int controller::create_influx_dbs(cyng::object &&cfg, std::string const &cmd) {

        auto const reader = cyng::make_reader(std::move(cfg));
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
        cyng::param_map_t &&pm,
        std::string const &name) {
        if (!pm.empty()) {
            CYNG_LOG_INFO(logger, "start sml database writer");
            auto const reader = cyng::make_reader(pm);
            auto channel = ctl.create_named_channel_with_ref<sml_db_writer>(name, ctl, logger);
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }
    void controller::start_sml_xml(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::param_map_t &&pm,
        std::string const &name) {
        if (!pm.empty()) {
            CYNG_LOG_INFO(logger, "start sml xml writer");
            auto channel = ctl.create_named_channel_with_ref<sml_xml_writer>(name, ctl, logger);
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }
    void controller::start_sml_json(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::param_map_t &&pm,
        std::string const &name) {
        if (!pm.empty()) {
            CYNG_LOG_INFO(logger, "start sml json writer");
            auto channel = ctl.create_named_channel_with_ref<sml_json_writer>(name, ctl, logger);
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }
    void controller::start_sml_abl(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::param_map_t &&pm,
        std::string const &name) {
        if (!pm.empty()) {
            CYNG_LOG_INFO(logger, "start sml abl writer");
            auto channel = ctl.create_named_channel_with_ref<sml_abl_writer>(name, ctl, logger);
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }
    void controller::start_sml_log(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::param_map_t &&pm,
        std::string const &name) {
        if (!pm.empty()) {
            CYNG_LOG_INFO(logger, "start sml log writer");
            auto channel = ctl.create_named_channel_with_ref<sml_log_writer>(name, ctl, logger);
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }
    void controller::start_sml_csv(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::param_map_t &&pm,
        std::string const &name) {
        if (!pm.empty()) {
            CYNG_LOG_INFO(logger, "start sml csv writer");
            auto channel = ctl.create_named_channel_with_ref<sml_csv_writer>(name, ctl, logger);
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
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
        cyng::param_map_t &&pm,
        std::string const &name) {
        if (!pm.empty()) {
            CYNG_LOG_INFO(logger, "start iec database writer");
            auto channel = ctl.create_named_channel_with_ref<iec_db_writer>(name, ctl, logger);
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }
    void controller::start_iec_log(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::param_map_t &&pm,
        std::string const &name) {
        if (!pm.empty()) {
            CYNG_LOG_INFO(logger, "start iec log writer");
            auto channel = ctl.create_named_channel_with_ref<iec_log_writer>(name, ctl, logger);
            BOOST_ASSERT(channel->is_open());
            channels.lock(channel);
        }
    }
    void controller::start_iec_csv(
        cyng::controller &ctl,
        cyng::stash &channels,
        cyng::logger logger,
        cyng::param_map_t &&pm,
        std::string const &name) {
        if (!pm.empty()) {
            CYNG_LOG_INFO(logger, "start iec csv writer");
            auto channel = ctl.create_named_channel_with_ref<iec_csv_writer>(name, ctl, logger);
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
