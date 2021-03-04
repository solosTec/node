/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <controller.h>
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
				create_client_spec(),
				create_cluster_spec()
			)
		});
	}

	void controller::run(cyng::controller& ctl, cyng::logger logger, cyng::object const& cfg) {
#if _DEBUG_BROKER_IEC
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
			, std::move(tgl));

	}

	void controller::join_cluster(cyng::controller& ctl
		, cyng::logger logger
		, boost::uuids::uuid tag
		, toggle::server_vec_t&& tgl) {

		//auto channel = ctl.create_named_channel_with_ref<cluster>("cluster", ctl, tag, logger, std::move(tgl));
		//BOOST_ASSERT(channel->is_open());
		//channel->dispatch("connect", cyng::make_tuple());
		//channel->dispatch("status_check", cyng::make_tuple(1));
	}

	cyng::param_t controller::create_server_spec(std::filesystem::path const& cwd) {
		return cyng::make_param("server", cyng::make_tuple(
			cyng::make_param("address", "0.0.0.0"),
			cyng::make_param("service", "12001")
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

	cyng::param_t controller::create_client_spec() {
		return cyng::make_param("client", cyng::make_tuple(
			cyng::make_param("login", false),
			cyng::make_param("verbose", false),	//	parser	
			cyng::make_param("target", "power@solostec")	//	push target name

		));
	}

}