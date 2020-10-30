/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/cluster.h"
#include <NODE_project_info.h>
#ifdef NODE_SSL_INSTALLED
#include <smf/http/srv/auth.h>
#endif
#include <cyng/async/task/task_builder.hpp>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/io/io_bytes.hpp>
#include <cyng/json.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/numeric_cast.hpp>
#include <cyng/vector_cast.hpp>
#include <cyng/set_cast.h>
#include <cyng/rnd.h>

#include <boost/uuid/uuid_io.hpp>

namespace node 
{
	//
	//	forward declaration(s):
	//
	void join_cluster(cyng::async::mux&
		, cyng::logging::log_ptr
		, boost::uuids::uuid
		, cyng::vector_t
		, cyng::tuple_t);

	controller::controller(unsigned int index
		, unsigned int pool_size
		, std::string const& json_path
		, std::string node_name)
	: ctl(index
		, pool_size
		, json_path
		, node_name)
	{}

	cyng::vector_t controller::create_config(std::fstream& fout, cyng::filesystem::path&& tmp, cyng::filesystem::path&& cwd) const
	{
		auto const root = (cwd / ".." / "dash" / "dist").lexically_normal();
		boost::uuids::random_generator uidgen;

		//
		//	generate even distributed integers
		//	reconnect to master on different times
		//
		cyng::crypto::rnd_num<int> rng(10, 120);

		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
				, cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", uidgen())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
				, cyng::param_factory("oui", (cwd / "oui.csv").string())

				, cyng::param_factory("server", cyng::tuple_factory(
					cyng::param_factory("address", "0.0.0.0"),
					cyng::param_factory("service", "8080"),
					cyng::param_factory("timeout", "15"),	//	seconds
					cyng::param_factory("max-upload-size", 1024 * 1024 * 10),	//	10 MB
					cyng::param_factory("document-root", root.string()),
					cyng::param_factory("server-nickname", "Coraline"),	//	x-servernickname
#ifdef NODE_SSL_INSTALLED
					cyng::param_factory("auth", cyng::vector_factory({
						//	directory: /
						//	authType:
						//	user:
						//	pwd:
						cyng::tuple_factory(
							cyng::param_factory("directory", "/"),
							cyng::param_factory("authType", "Basic"),
							cyng::param_factory("realm", "solos::Tec"),
							cyng::param_factory("name", "auth@example.com"),
							cyng::param_factory("pwd", "secret")
						),
						cyng::tuple_factory(
							cyng::param_factory("directory", "/temp"),
							cyng::param_factory("authType", "Basic"),
							cyng::param_factory("realm", "Restricted Content"),
							cyng::param_factory("name", "auth@example.com"),
							cyng::param_factory("pwd", "secret")
						) }
					)),	//	auth
#else
						cyng::param_factory("auth-support", false),
#endif
					cyng::param_factory("blocklist", cyng::vector_factory({
						//	https://bl.isx.fr/raw
						cyng::make_address("185.244.25.187"),	//	KV Solutions B.V. scans for "login.cgi"
						cyng::make_address("139.219.100.104"),	//	ISP Microsoft (China) Co. Ltd. - 2018-07-31T21:14
						cyng::make_address("194.147.32.109"),	//	Udovikhin Evgenii - 2019-02-01 15:23:08.13699453
						cyng::make_address("185.209.0.12"),		//	2019-03-27 11:23:39
						cyng::make_address("42.236.101.234"),	//	hn.kd.ny.adsl (china)
						cyng::make_address("185.104.184.126"),	//	M247 Ltd
						cyng::make_address("185.162.235.56"),	//	SILEX malware
						cyng::make_address("185.202.2.147")		//	awesouality.com - brute force 2020-10-30
						})),	//	blocklist
					cyng::param_factory("redirect-specific", cyng::tuple_factory(
						//	specific redirects 
						cyng::param_factory("/", "/index.html")
					)),
					cyng::param_factory("redirect-generic", cyng::tuple_factory(
						//	generic redirects (if starts with ..., then
						cyng::param_factory("/config", "/index.html"),
						cyng::param_factory("/status", "/index.html"),
						cyng::param_factory("/monitor", "/index.html"),
						cyng::param_factory("/task", "/index.html"),
						cyng::param_factory("/collector", "/index.html")
					)),
					cyng::param_factory("https-rewrite", false)	//	301 - Moved Permanently
				))

				, cyng::param_factory("cluster", cyng::vector_factory({ cyng::tuple_factory(
					cyng::param_factory("host", "127.0.0.1"),
					cyng::param_factory("service", "7701"),
					cyng::param_factory("account", "root"),
					cyng::param_factory("pwd", NODE_PWD),
					cyng::param_factory("salt", NODE_SALT),
					cyng::param_factory("monitor", rng()),	//	seconds
					cyng::param_factory("auto-config", false),	//	client security
					cyng::param_factory("group", 0)	//	customer ID
				) }))
			)
		});
	}

	bool controller::start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::reader<cyng::object> const& cfg, boost::uuids::uuid tag)
	{
		//
		//	connect to cluster
		//
		join_cluster(mux
			, logger
			, tag
			, cyng::to_vector(cfg.get("cluster"))
			, cyng::to_tuple(cfg.get("server")));

		//
		//	wait for system signals
		//
		return wait(logger);
	}


	void join_cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid cluster_tag
		, cyng::vector_t cfg_cls
		, cyng::tuple_t cfg_srv)
	{
		CYNG_LOG_TRACE(logger, "cluster redundancy: " << cfg_cls.size());

		auto dom = cyng::make_reader(cfg_srv);

		auto const pwd = cyng::filesystem::current_path();
		auto const root = (pwd / ".." / "dash" / "dist").lexically_normal();

		//	http::server build a string view
		static auto doc_root = cyng::value_cast(dom.get("document-root"), root.string());
		auto address = cyng::value_cast<std::string>(dom.get("address"), "0.0.0.0");
		auto service = cyng::value_cast<std::string>(dom.get("service"), "8080");
		auto const host = cyng::make_address(address);
		auto const port = static_cast<unsigned short>(std::stoi(service));
		auto const timeout = cyng::numeric_cast<std::size_t>(dom.get("timeout"), 15u);
		auto const max_upload_size = cyng::numeric_cast<std::uint64_t>(dom.get("max-upload-size"), 1024u * 1024 * 10u);
		auto const nickname = cyng::value_cast<std::string>(dom.get("server-nickname"), "Coraline");

		cyng::error_code ec;
		if (cyng::filesystem::exists(doc_root, ec)) {
			CYNG_LOG_INFO(logger, "document root: [" << doc_root << "]");
		}
		else {
			CYNG_LOG_FATAL(logger, "document root does not exists: [" << doc_root << "]");
		}
		CYNG_LOG_INFO(logger, "address: " << address);
		CYNG_LOG_INFO(logger, "service: " << service);
		CYNG_LOG_INFO(logger, "timeout: " << timeout << " seconds");
		if (max_upload_size < 10 * 1024) {
			CYNG_LOG_WARNING(logger, "max-upload-size only: " << max_upload_size << " bytes");
		}
		else {
			CYNG_LOG_INFO(logger, "max-upload-size: " << cyng::bytes_to_str(max_upload_size));
		}

		auto const oui = cyng::value_cast<std::string>(dom.get("oui"), "oui.csv");
		if (cyng::filesystem::exists(oui, ec)) {
			CYNG_LOG_INFO(logger, "oui db: [" << oui << "]");
		}
		else {
			CYNG_LOG_ERROR(logger, "oui db does not exists: [" << oui << "]");
		}

#ifdef NODE_SSL_INSTALLED
		//
		//	get user credentials
		//
		auth_dirs ad;
		init(dom.get("auth"), ad);
		for (auto const& dir : ad) {
			CYNG_LOG_INFO(logger, "restricted access to [" << dir.first << "]");
		}
#endif
		auto const https_rewrite = cyng::value_cast(dom.get("https-rewrite"), false);
		if (https_rewrite) {
			CYNG_LOG_WARNING(logger, "HTTPS rewrite is active");
		}

		//
		//	get blocklisted addresses
		//
		const auto blocklist_str = cyng::vector_cast<std::string>(dom.get("blocklist"), "");
		CYNG_LOG_INFO(logger, blocklist_str.size() << " addresses are blocklisted");
		std::set<boost::asio::ip::address>	blocklist;
		for (auto const& a : blocklist_str) {
			auto r = blocklist.insert(boost::asio::ip::make_address(a));
			if (r.second) {
				CYNG_LOG_TRACE(logger, *r.first);
			}
			else {
				CYNG_LOG_WARNING(logger, "cannot insert " << a);
			}
		}

		//
		//	specific redirects
		//
		std::map<std::string, std::string> redirects_specific;
		{
			auto const rs = cyng::to_param_map(dom.get("redirect-specific"));	// cyng::param_map_t
			CYNG_LOG_INFO(logger, rs.size() << " specific redirects configured");
			std::transform(std::begin(rs), std::end(rs), std::inserter(redirects_specific, redirects_specific.begin()), [](auto const& redirect) -> std::map<std::string, std::string>::value_type {
				return { redirect.first, cyng::value_cast<std::string>(redirect.second, "") };
				});
		}

		//
		//	generic redirects
		//
		std::map<std::string, std::string> redirects_generic;
		{
			auto const rg = cyng::to_param_map(dom.get("redirect-generic"));	// cyng::param_map_t
			CYNG_LOG_INFO(logger, rg.size() << " generic redirects configured");
			std::transform(std::begin(rg), std::end(rg), std::inserter(redirects_generic, redirects_generic.begin()), [](auto const& redirect) -> std::map<std::string, std::string>::value_type {
				return { redirect.first, cyng::value_cast<std::string>(redirect.second, "") };
				});
		}

		cyng::async::start_task_delayed<cluster>(mux
			, std::chrono::seconds(1)
			, logger
			, cluster_tag
			, load_cluster_cfg(cfg_cls)
			, boost::asio::ip::tcp::endpoint{ host, port }
			, timeout
			, max_upload_size
			, doc_root
			, nickname
#ifdef NODE_SSL_INSTALLED
			, ad
#endif
			, blocklist
			, oui
			, redirects_specific
			, redirects_generic
			, https_rewrite);

	}


}
