/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>

#include <smf.h>

#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/obj/object.h>
#include <cyng/sys/locale.h>
#include <cyng/io/ostream.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/log/record.h>
#include <cyng/task/controller.h>

#include <locale>
#include <iostream>

#include <boost/algorithm/string.hpp>


namespace smf {

	controller::controller(config::startup const& config) 
		: controller_base(config)
	{}

	cyng::vector_t controller::create_default_config(std::chrono::system_clock::time_point&& now
		, std::filesystem::path&& tmp
		, std::filesystem::path&& cwd) {

		std::locale loc(std::locale(), new std::ctype<char>);
		std::cout << std::locale("").name().c_str() << std::endl;

		return cyng::make_vector({
			cyng::make_tuple(
				cyng::make_param("generated", now),
				cyng::make_param("log-dir", tmp.string()),
				cyng::make_param("tag", get_random_tag()),
				cyng::make_param("country-code", "CH"),
				cyng::make_param("language-code", cyng::sys::get_system_locale()),

				cyng::make_param("output", cyng::make_vector({"ALL:BIN"})),	//	options are XML, JSON, DB, BIN, ...

				 cyng::make_param("SML:DB", cyng::tuple_factory(
					cyng::make_param("type", "SQLite"),
					cyng::make_param("file-name", (cwd / "store.database").string()),
					cyng::make_param("busy-timeout", 12),		//	seconds
					cyng::make_param("watchdog", 30),	//	for database connection
					cyng::make_param("pool-size", 1),	//	no pooling for SQLite
					cyng::make_param("db-schema", SMF_VERSION_NAME),		//	use "v4.0" for compatibility to version 4.x
					cyng::make_param("interval", 12)	//	seconds
				)),
				 cyng::make_param("IEC:DB", cyng::tuple_factory(
					cyng::make_param("type", "SQLite"),
					cyng::make_param("file-name", (cwd / "store.database").string()),
					cyng::make_param("busy-timeout", 12),		//	seconds
					cyng::make_param("watchdog", 30),	//	for database connection
					cyng::make_param("pool-size", 1),	//	no pooling for SQLite
					cyng::make_param("db-schema", SMF_VERSION_NAME),
					cyng::make_param("interval", 20),	//	seconds
					cyng::make_param("ignore-null", false)	//	don't write values equal 0
				)),
				 cyng::make_param("SML:XML", cyng::tuple_factory(
					cyng::make_param("root-dir", (cwd / "xml").string()),
					cyng::make_param("root-name", "SML"),
					cyng::make_param("endcoding", "UTF-8"),
					cyng::make_param("interval", 18)	//	seconds
				)),
				 cyng::make_param("SML:JSON", cyng::tuple_factory(
					cyng::make_param("root-dir", (cwd / "json").string()),
					cyng::make_param("prefix", "smf"),
					cyng::make_param("suffix", "json"),
					cyng::make_param("version", SMF_VERSION_NAME),
					cyng::make_param("interval", 21)	//	seconds
				)),
				 cyng::make_param("SML:ABL", cyng::tuple_factory(
					cyng::make_param("root-dir", (cwd / "abl").string()),
					cyng::make_param("prefix", "smf"),
					cyng::make_param("suffix", "abl"),
					cyng::make_param("version", SMF_VERSION_NAME),
					cyng::make_param("interval", 33),	//	seconds
					cyng::make_param("line-ending", "DOS")	//	DOS/UNIX
				)),
				 cyng::make_param("ALL:BIN", cyng::tuple_factory(
					cyng::make_param("root-dir", (cwd / "sml").string()),
					cyng::make_param("prefix", "smf"),
					cyng::make_param("suffix", "sml"),
					cyng::make_param("version", SMF_VERSION_NAME),
					cyng::make_param("interval", 27)
				)),
				 cyng::make_param("SML:LOG", cyng::tuple_factory(
					cyng::make_param("root-dir", (cwd / "log").string()),
					cyng::make_param("prefix", "sml"),
					cyng::make_param("suffix", "log"),
					cyng::make_param("version", SMF_VERSION_NAME),
					cyng::make_param("interval", 26)	//	seconds
				)),
				 cyng::make_param("IEC:LOG", cyng::tuple_factory(
					cyng::make_param("root-dir", (cwd / "log").string()),
					cyng::make_param("prefix", "iec"),
					cyng::make_param("suffix", "log"),
					cyng::make_param("version", SMF_VERSION_NAME),
					cyng::make_param("interval", 16)	//	seconds
				)),
				 cyng::make_param("SML:CSV", cyng::tuple_factory(
					cyng::make_param("root-dir", (cwd / "csv").string()),
					cyng::make_param("prefix", "smf"),
					cyng::make_param("suffix", "csv"),
					cyng::make_param("header", true),
					cyng::make_param("version", SMF_VERSION_NAME),
					cyng::make_param("interval", 39)	//	seconds
				)),
				 cyng::make_param("SML:influxdb", cyng::tuple_factory(
					cyng::make_param("host", "localhost"),
					cyng::make_param("port", "8086"),	//	8094 for udp
					cyng::make_param("protocol", "http"),	//	http, https, udp, unix
					cyng::make_param("cert", (cwd / "cert.pem").string()),
					cyng::make_param("db", "SMF"),
					cyng::make_param("series", "SML"),
					cyng::make_param("auth-enabled", false),
					cyng::make_param("user", "user"),
					cyng::make_param("pwd", "pwd"),
					cyng::make_param("interval", 46)	//	seconds
				)),

				cyng::make_param("server", cyng::make_tuple(
					cyng::make_param("address", "0.0.0.0"),
					cyng::make_param("service", "26862"),
					cyng::make_param("sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
					cyng::make_param("watchdog", 30),	//	for IP-T connection (minutes)
					cyng::make_param("timeout", 10)		//	connection timeout in seconds
				)),
				cyng::make_param("ipt", cyng::make_vector({
					cyng::make_tuple(
						cyng::make_param("host", "127.0.0.1"),
						cyng::make_param("service", "26862"),
						cyng::make_param("account", "data-store"),
						cyng::make_param("pwd", "to-define"),
						cyng::make_param("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::make_param("scrambled", true)
						//cyng::param_factory("monitor", rng())),	//	seconds
					),
					cyng::make_tuple(
						cyng::make_param("host", "127.0.0.1"),
						cyng::make_param("service", "26863"),
						cyng::make_param("account", "data-store"),
						cyng::make_param("pwd", "to-define"),
						cyng::make_param("def-sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
						cyng::make_param("scrambled", false)
						//cyng::make_param("monitor", rng()))
						//cyng::make_param("monitor", rng())),	//	seconds
					)})
				),
				cyng::make_param("targets", cyng::make_vector({
					//	list of targets and their data type
					cyng::make_param("water@solostec", "SML"),
					cyng::make_param("gas@solostec", "SML"),
					cyng::make_param("power@solostec", "SML"),
					cyng::make_param("LZQJ", "IEC") 
				}))
			)
			});
	}
	void controller::run(cyng::controller&, cyng::logger, cyng::object const& cfg, std::string const& node_name) {

	}

	bool controller::run_options(boost::program_options::variables_map& vars) {

		//
		//	
		//
		if (vars["init"].as< bool >()) {
			//	initialize database
			init_storage(read_config_section(config_.json_path_, config_.config_index_));
			return true;
		}
		auto const cmd = vars["influxdb"].as< std::string >();
		if (!cmd.empty()) 	{
			//	execute a influx db command
			std::cerr << cmd << " not implemented yet" << std::endl;
			//return ctrl.create_influx_dbs(config_index, cmd);
			create_influx_dbs(read_config_section(config_.json_path_, config_.config_index_), cmd);
			return true;
		}
		//

		//
		//	call base classe
		//
		return controller_base::run_options(vars);

	}

	void controller::init_storage(cyng::object&& cfg) {

		auto const reader = cyng::make_reader(std::move(cfg));
		//auto s = cyng::db::create_db_session(reader.get("DB"));
		//if (s.is_alive())	smf::init_storage(s);	//	see task/storage_db.h
	}

	void controller::create_influx_dbs(cyng::object&& cfg, std::string const& cmd) {

		auto const reader = cyng::make_reader(std::move(cfg));
		auto const pmap = cyng::container_cast<cyng::param_map_t>(reader.get("SML:influxdb"));
		std::cout
			<< "***info: "
			<< pmap
			<< std::endl;

		cyng::controller ctl(config_.pool_size_);

		if (boost::algorithm::equals(cmd, "create")) {

			//
			//	create database
			//
			//return sml::influxdb_exporter::create_db(mux.get_io_service()
			//	, cyng::from_param_map<std::string>(cm, "host", "localhost")
			//	, cyng::from_param_map<std::string>(cm, "port", "8086")
			//	, cyng::from_param_map(cm, "protocol", "http")
			//	, cyng::from_param_map(cm, "cert", "cert.pem")
			//	, cyng::from_param_map<std::string>(cm, "db", "SMF"));
		}
		else if (boost::algorithm::equals(cmd, "show")) {
			//
			//	show database
			//
			//return sml::influxdb_exporter::show_db(mux.get_io_service()
			//	, cyng::from_param_map<std::string>(cm, "host", "localhost")
			//	, cyng::from_param_map<std::string>(cm, "port", "8086")
			//	, cyng::from_param_map(cm, "protocol", "http")
			//	, cyng::from_param_map(cm, "cert", "cert.pem"));
		}
		else if (boost::algorithm::equals(cmd, "drop")) {
			//
			//	drop database
			//
			//return sml::influxdb_exporter::drop_db(mux.get_io_service()
			//	, cyng::from_param_map<std::string>(cm, "host", "localhost")
			//	, cyng::from_param_map<std::string>(cm, "port", "8086")
			//	, cyng::from_param_map(cm, "protocol", "http")
			//	, cyng::from_param_map(cm, "cert", "cert.pem")
			//	, cyng::from_param_map<std::string>(cm, "db", "SMF"));
		}
		else {
			std::cerr
				<< "***error: unknown command "
				<< cmd
				<< std::endl;
		}

		ctl.stop();

	}

}