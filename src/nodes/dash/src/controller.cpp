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
#include <cyng/obj/vector_cast.hpp>
#include <cyng/obj/util.hpp>
#include <cyng/obj/object.h>
#include <cyng/sys/locale.h>
#include <cyng/io/ostream.h>
#include <cyng/obj/algorithm/reader.hpp>
#include <cyng/log/record.h>
#include <cyng/task/controller.h>

#include <locale>
#include <iostream>
#include <algorithm>
#include <iterator>

namespace smf {

	controller::controller(config::startup const& config) 
		: controller_base(config)
	{}

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

		auto const address = cyng::value_cast(reader["server"]["address"].get(), "0.0.0.0");
		auto const port = cyng::numeric_cast<std::uint16_t>(reader["server"]["port"].get(), 8080);
		auto const document_root = cyng::value_cast(reader["server"]["document-root"].get(), "");
		auto const max_upload_size = cyng::numeric_cast<std::uint64_t>(reader["server"]["max-upload-size"].get(), 10485760);
		auto const nickname = cyng::value_cast(reader["server"]["server-nickname"].get(), "");
		auto const timeout = std::chrono::seconds(cyng::numeric_cast<std::uint64_t>(reader["server"]["timeout"].get(), 15));

		//
		//	get blocklisted addresses
		//
		blocklist_type blocklist = convert_to_blocklist(cyng::vector_cast<std::string>(reader["server"]["blocklist"].get(), "0.0.0.0"));

		//
		//	connect to cluster
		//
		join_cluster(ctl
			, logger
			, tag
			, node_name
			, std::move(tgl)
			, address
			, port
			, document_root
			, max_upload_size
			, nickname
			, timeout
			, std::move(blocklist));

		
	}

	void controller::join_cluster(cyng::controller& ctl
		, cyng::logger logger
		, boost::uuids::uuid tag
		, std::string const& node_name
		, toggle::server_vec_t&& cfg
		, std::string const& address
		, std::uint16_t port
		, std::string const& document_root
		, std::uint64_t max_upload_size
		, std::string const& nickname
		, std::chrono::seconds timeout
		, blocklist_type&& blocklist) 
	{

		if (!std::filesystem::exists(document_root)) {
			CYNG_LOG_ERROR(logger, "document root [" << document_root << "] does not exists");
		}

		auto channel = ctl.create_named_channel_with_ref<cluster>("cluster"
			, ctl
			, tag
			, node_name
			, logger
			, std::move(cfg)
			, document_root
			, max_upload_size
			, nickname
			, timeout		
			, std::move(blocklist));

		BOOST_ASSERT(channel->is_open());
		channel->dispatch("connect", cyng::make_tuple());

		auto const ep = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port);
		channel->dispatch("listen", cyng::make_tuple(ep));
	}

	cyng::vector_t controller::create_default_config(std::chrono::system_clock::time_point&& now
		, std::filesystem::path&& tmp
		, std::filesystem::path&& cwd) {

		auto const root = (cwd / ".." / "dash" / "dist").lexically_normal();

		return cyng::make_vector({
			cyng::make_tuple(
				cyng::make_param("generated", now),
				cyng::make_param("log-dir", tmp.string()),
				cyng::make_param("tag", get_random_tag()),
				cyng::make_param("country-code", "CH"),
				cyng::make_param("language-code", cyng::sys::get_system_locale()),
				create_server_spec(root),
				create_cluster_spec()
			)
		});
	}

	cyng::param_t controller::create_server_spec(std::filesystem::path const& root) {
		return cyng::make_param("server", cyng::make_tuple(
			cyng::make_param("address", "0.0.0.0"),
			cyng::make_param("port", 8080),
			cyng::make_param("timeout", "15"),	//	seconds
			cyng::make_param("max-upload-size", 1024 * 1024 * 10),	//	10 MB
			cyng::make_param("document-root", root.string()),
			cyng::make_param("server-nickname", "Coraline"),	//	x-servernickname
			create_auth_spec(),
			create_block_list()
			//create_redirects()	//	ToDo: create redirect rules
		));
	}

	cyng::param_t controller::create_block_list() {
		boost::system::error_code ec;
		return cyng::make_param("blocklist", cyng::make_vector({
			boost::asio::ip::make_address("185.244.25.187", ec),	//	KV Solutions B.V. scans for "login.cgi"
			boost::asio::ip::make_address("139.219.100.104", ec),	//	ISP Microsoft (China) Co. Ltd. - 2018-07-31T21:14
			boost::asio::ip::make_address("194.147.32.109", ec),	//	Udovikhin Evgenii - 2019-02-01 15:23:08.13699453
			boost::asio::ip::make_address("185.209.0.12", ec),		//	2019-03-27 11:23:39
			boost::asio::ip::make_address("42.236.101.234", ec),	//	hn.kd.ny.adsl (china)
			boost::asio::ip::make_address("185.104.184.126", ec),	//	M247 Ltd
			boost::asio::ip::make_address("185.162.235.56", ec),	//	SILEX malware
			boost::asio::ip::make_address("185.202.2.147", ec)		//	awesouality.com - brute force 2020-10-30
			}
		));
	}

	cyng::param_t controller::create_auth_spec() {
		return cyng::make_param("auth", cyng::make_vector({
			cyng::make_tuple(
				cyng::make_param("directory", "/"),
				cyng::make_param("authType", "Basic"),
				cyng::make_param("realm", "solos::Tec"),
				cyng::make_param("name", "auth@example.com"),
				cyng::make_param("pwd", "secret")),
			cyng::make_tuple(
				cyng::make_param("directory", "/temp"),
				cyng::make_param("authType", "Basic"),
				cyng::make_param("realm", "solos::Tec"),
				cyng::make_param("name", "auth@example.com"),
				cyng::make_param("pwd", "secret"))
			}
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

	std::vector<boost::asio::ip::address> convert_to_blocklist(std::vector<std::string>&& inp) {
		std::vector<boost::asio::ip::address> res;
		res.reserve(inp.size());
		std::transform(std::begin(inp), std::end(inp), std::back_inserter(res), [](std::string const& s) {
			return boost::asio::ip::make_address(s);
			});
		return res;
	}

}