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
#include <cyng/task/registry.h>

#include <locale>
#include <iostream>

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
				create_server_spec(cwd),
				create_cluster_spec()
			)
		});
	}
	void controller::run(cyng::controller&, cyng::logger, cyng::object const& cfg, std::string const& node_name) {

	}
	void controller::shutdown(cyng::logger logger, cyng::registry& reg) {

		//
		//	stop all running tasks
		//
		reg.shutdown();
	}

	cyng::param_t controller::create_server_spec(std::filesystem::path const& cwd) {
		return cyng::make_param("server", cyng::make_tuple(
			cyng::make_param("address", "0.0.0.0"),
			cyng::make_param("service", "8443"),
			cyng::make_param("timeout", "15"),	//	seconds
			cyng::make_param("max-upload-size", 1024 * 1024 * 10),	//	10 MB
			cyng::make_param("document-root", (cwd / "htdocs").string()),
			cyng::make_param("tls-pwd", "test"),
			cyng::make_param("tls-certificate-chain", "fullchain.cert"),
			cyng::make_param("tls-private-key", "privkey.key"),
			cyng::make_param("tls-dh", "dh4096.dh")	//	diffie-hellman
			));
	}

	cyng::param_t controller::create_cluster_spec() {
		return cyng::make_param("cluster", cyng::make_vector({
			cyng::make_tuple(
				cyng::make_param("host", "127.0.0.1"),
				cyng::make_param("service", "7701"),
				cyng::make_param("account", "root"),
				cyng::make_param("pwd", "NODE_PWD"),
				cyng::make_param("salt", "NODE_SALT"),
				//cyng::make_param("monitor", rnd_monitor()),	//	seconds
				cyng::make_param("group", 0))	//	customer ID
			}
		));
	}

}