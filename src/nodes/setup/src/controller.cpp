/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>
#include <tasks/cluster.h>

#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/obj/object.h>
#include <cyng/sys/locale.h>
#include <cyng/io/ostream.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/log/record.h>
#include <cyng/task/controller.h>

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

				cyng::make_param("input", "DB"),	//	options are XML, JSON, DB

				cyng::make_param("DB", cyng::make_tuple(
					cyng::make_param("type", "SQLite"),
					cyng::make_param("file-name", (cwd / "setup.database").string()),
					cyng::make_param("busy-timeout", 12),		//	seconds
					cyng::make_param("watchdog", 30),		//	for database connection
					cyng::make_param("pool-size", 1)		//	no pooling for SQLite
				)),

				cyng::make_param("XML", cyng::make_tuple(
					cyng::make_param("file-name", (cwd / "setup.config.xml").string()),
					cyng::make_param("endcoding", "UTF-8")
				)),

				cyng::make_param("JSON", cyng::tuple_factory(
					cyng::make_param("file-name", (cwd / "setup.config.json").string())
				)),

				create_cluster_spec()
			)
		});
	}

	void controller::run(cyng::controller& ctl, cyng::logger logger, cyng::object const& cfg, std::string const& node_name) {
#if _DEBUG_DASH
		CYNG_LOG_INFO(logger, cfg);
#endif
		auto const reader = cyng::make_reader(cfg);
		auto const tag = cyng::value_cast(reader["tag"].get(), this->get_random_tag());

		auto tgl = read_config(cyng::container_cast<cyng::vector_t>(reader["cluster"].get()));
		BOOST_ASSERT(!tgl.empty());
		if (tgl.empty()) {
			CYNG_LOG_FATAL(logger, "no cluster data configured");
		}

		//
		//	connect to cluster
		//
		join_cluster(ctl
			, logger
			, tag
			, node_name
			, std::move(tgl));

	}

	void controller::join_cluster(cyng::controller& ctl
		, cyng::logger logger
		, boost::uuids::uuid tag
		, std::string const& node_name
		, toggle::server_vec_t&& cfg) {

		auto channel = ctl.create_named_channel_with_ref<cluster>("cluster", ctl, tag, node_name, logger, std::move(cfg));
		BOOST_ASSERT(channel->is_open());
		channel->dispatch("connect", cyng::make_tuple());
		channel->dispatch("status_check", cyng::make_tuple(1));
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