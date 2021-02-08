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
				cyng::make_param("DB", cyng::make_tuple(
//#if defined(NODE_CROSS_COMPILE) && BOOST_OS_LINUX
//					cyng::make_param("file-name", "/usr/local/etc/smf/segw.database"),
//#else
					cyng::make_param("file-name", (cwd / "segw.database").string()),
//#endif
					cyng::make_param("busy-timeout", 12),		//	seconds
					cyng::make_param("watchdog", 30),		//	for database connection
					cyng::make_param("pool-size", 1)		//	no pooling for SQLite
				)),
				cyng::make_param("sml", cyng::make_tuple(
					cyng::make_param("address", "0.0.0.0"),
					cyng::make_param("service", "7259"),
					cyng::make_param("discover", "5798"),	//	UDP
					cyng::make_param("account", "operator"),
					cyng::make_param("pwd", "operator"),
					//cyng::make_param("enabled", bpl ? false : true),
					cyng::make_param("accept-all-ids", false)	//	accept only the specified MAC id
				)),
				cyng::make_param("nms", cyng::tuple_factory(
					cyng::make_param("address", "0.0.0.0"),
					cyng::make_param("port", 7261),
					cyng::make_param("account", "operator"),
					cyng::make_param("pwd", "operator"),
					//cyng::make_param("enabled", bpl ? true : false),
					cyng::make_param("script-path",
#if BOOST_OS_LINUX
						(tmp / "update-script.sh").string()
#else
						(tmp / "update-script.cmd").string()
#endif
					)
				))
			)
		});
	}
	void controller::print_configuration(std::ostream& os) {
		os << "ToDo" << std::endl;
	}

}