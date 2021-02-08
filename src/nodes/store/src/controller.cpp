/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/obj/object.h>
#include <cyng/sys/locale.h>

#include <locale>
#include <iostream>

namespace smf {

	controller::controller(config::startup const& config) 
		: controller_base(config)
	{}

	int controller::run() {
		return EXIT_SUCCESS;
	}

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
	void controller::print_configuration(std::ostream& os) {
		os << "ToDo" << std::endl;
	}

}