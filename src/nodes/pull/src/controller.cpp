/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */

#include <controller.h>

#include <tasks/network.h>

#include <smf.h>
//#include <smf/obis/db.h>
//#include <smf/obis/defs.h>
//#include <smf/obis/profile.h>

#include <cyng/io/ostream.h>
#include <cyng/log/record.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
//#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/object.h>
#include <cyng/obj/set_cast.hpp>
//#include <cyng/obj/util.hpp>
//#include <cyng/obj/vector_cast.hpp>
//#include <cyng/parse/string.h>
#include <cyng/sys/clock.h>
#include <cyng/sys/locale.h>
//#include <cyng/task/controller.h>

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
            cyng::make_param("utc-offset", cyng::sys::delta_utc(now).count()),
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
                "ipt",
                cyng::make_vector(
                    {cyng::make_tuple(
                         cyng::make_param("host", "localhost"),
                         cyng::make_param("service", "26862"),
                         cyng::make_param("account", "pull-engine"),
                         cyng::make_param("pwd", "to-define"),
                         cyng::make_param(
                             "def-sk", "0102030405060708090001020304050607080900010203040506070809000001"), //	scramble key
                         cyng::make_param("scrambled", true)),
                     cyng::make_tuple(
                         cyng::make_param("host", "localhost"),
                         cyng::make_param("service", "26863"),
                         cyng::make_param("account", "pull-engine"),
                         cyng::make_param("pwd", "to-define"),
                         cyng::make_param(
                             "def-sk", "0102030405060708090001020304050607080900010203040506070809000001"), //	scramble key
                         cyng::make_param("scrambled", false))})),
            cyng::make_param(
                "targets", cyng::make_vector({"water@solostec", "gas@solostec", "power@solostec"}) //
                )                                                                                  // targets
            )});
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
        auto sm = init_storage(cfg, false);

        //
        //  seconds to wait before starting ip-t client
        //
        auto const delay = cyng::numeric_cast<std::uint32_t>(reader["network-delay"].get(), 6);
        CYNG_LOG_INFO(logger, "start ipt bus in " << delay << " seconds");

        //  No duplicates possible by using a set
        auto const targets = cyng::set_cast<std::string>(reader.get("targets"), "sml@store");
        if (targets.empty()) {
            CYNG_LOG_WARNING(logger, "no IP-T targets configured");
        } else {
            CYNG_LOG_TRACE(logger, targets.size() << " IP-T targets configured");
        }

        //
        //  connect to ip-t server
        //
        join_network(ctl, channels, logger, tag, node_name, model, std::move(tgl), std::chrono::seconds(delay), targets);
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
        std::set<std::string> const &targets) {

        auto channel = ctl.create_named_channel_with_ref<network>("network", ctl, tag, logger, node_name, model, std::move(tgl), targets);
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
                        // sml_db_writer::init_storage(pos->second);
                    } else {
                        std::cerr << "***error: writer " << name << " uses an undefined database: " << db << std::endl;
                    }
                } else if (boost::algorithm::equals(name, "IEC:DB")) {
                    auto const db = reader[name].get("db", "default");
                    auto const pos = sm.find(db);
                    if (pos != sm.end()) {
                        std::cout << "***info : writer " << name << " initialize database " << pos->first << std::endl;
                        // iec_db_writer::init_storage(pos->second);
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
        //	call base classe
        //
        return controller_base::run_options(vars);
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
            // iec_db_writer::init_storage(s); //	see task/iec_db_writer.h
            // sml_db_writer::init_storage(s); //	see task/sml_db_writer.h
            std::cout << "***info : database created" << std::endl;
        } else {
            std::cerr << "***error: create database failed: " << pm << std::endl;
        }
        return s;
    }

} // namespace smf
