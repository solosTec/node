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
				cyng::make_param("server", cyng::make_tuple(
					cyng::make_param("address", "0.0.0.0"),
					cyng::make_param("service", "26862"),
					cyng::make_param("sk", "0102030405060708090001020304050607080900010203040506070809000001"),	//	scramble key
					cyng::make_param("watchdog", 30),	//	for IP-T connection (minutes)
					cyng::make_param("timeout", 10)		//	connection timeout in seconds
				)),
				cyng::make_param("cluster", cyng::make_tuple(
					cyng::make_param("host", "127.0.0.1"),
					cyng::make_param("service", "7701"),
					cyng::make_param("account", "root"),
					cyng::make_param("pwd", "NODE_PWD"),
					cyng::make_param("salt", "NODE_SALT"),
					//cyng::make_param("monitor", rnd_monitor()),	//	seconds
					cyng::make_param("group", 0)	//	customer ID
				))
			)
		});
	}
	void controller::print_configuration(std::ostream& os) {
		os << "ToDo" << std::endl;
	}

}