/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/cluster.h"
#include <NODE_project_info.h>
#include <smf/http/srv/auth.h>

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
	void join_cluster(cyng::logging::log_ptr
		, cyng::async::mux&
		, boost::asio::ssl::context& ctx
		, cyng::vector_t const&
		, cyng::tuple_t const&);
	bool load_server_certificate(boost::asio::ssl::context& ctx
		, cyng::logging::log_ptr
		, std::string const& tls_pwd
		, std::string const& tls_certificate_chain
		, std::string const& tls_private_key
		, std::string const& tls_dh);

	controller::controller(unsigned int index
		, unsigned int pool_size
		, std::string const& json_path
		, std::string node_name)
	: ctl(index
			, pool_size
			, json_path
			, node_name)
	{}

	cyng::vector_t controller::create_config(std::fstream& fout, boost::filesystem::path&& tmp, boost::filesystem::path&& cwd) const
	{
		auto const root = (cwd / ".." / "dash" / "dist").lexically_normal();

		//
		//	generate even distributed integers
		//	reconnect to master on different times
		//
		cyng::crypto::rnd_num<int> rng(10, 120);

		return cyng::vector_factory({
			cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
                , cyng::param_factory("log-level", "INFO")
				, cyng::param_factory("tag", uidgen_())
				, cyng::param_factory("generated", std::chrono::system_clock::now())
				, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))

				, cyng::param_factory("server", cyng::tuple_factory(
					cyng::param_factory("address", "0.0.0.0"),
					cyng::param_factory("service", "8443"),	//	default is 443
					cyng::param_factory("timeout", "15"),	//	seconds
					cyng::param_factory("max-upload-size", 1024 * 1024 * 10),	//	10 MB
					cyng::param_factory("document-root", root.string()),
					cyng::param_factory("tls-pwd", "test"),
					cyng::param_factory("tls-certificate-chain", "demo.cert"),
                    cyng::param_factory("tls-private-key", "priv.key"),
					cyng::param_factory("tls-dh", "demo.dh"),	//	diffie-hellman
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
					cyng::param_factory("blacklist", cyng::vector_factory({
						//	https://bl.isx.fr/raw
						cyng::make_address("185.244.25.187"),	//	KV Solutions B.V. scans for "login.cgi"
						cyng::make_address("139.219.100.104"),	//	ISP Microsoft (China) Co. Ltd. - 2018-07-31T21:14
						cyng::make_address("194.147.32.109"),	//	Udovikhin Evgenii - 2019-02-01 15:23:08.13699453
						cyng::make_address("185.209.0.12"),		//	2019-03-27 11:23:39
						cyng::make_address("42.236.101.234"),	//	hn.kd.ny.adsl (china)
						cyng::make_address("185.104.184.126"),	//	M247 Ltd
						cyng::make_address("185.162.235.56")	//	SILEX malware
						})),	//	blacklist
					cyng::param_factory("redirect", cyng::vector_factory({
						cyng::param_factory("/", "/index.html"),
						cyng::param_factory("/config/device", "/index.html" ),
						cyng::param_factory("/config/gateway", "/index.html" ),
						cyng::param_factory("/config/meter", "/index.html" ),
						cyng::param_factory("/config/lora", "/index.html" ),
						cyng::param_factory("/config/upload", "/index.html" ),
						cyng::param_factory("/config/download", "/index.html" ),
						cyng::param_factory("/config/system", "/index.html"),
						cyng::param_factory("/config/web", "/index.html"),

						cyng::param_factory("/status/sessions", "/index.html"),
						cyng::param_factory("/status/targets", "/index.html"),
						cyng::param_factory("/status/connections", "/index.html"),

						cyng::param_factory("/monitor/system", "/index.html"),
						cyng::param_factory("/monitor/messages", "/index.html"),
						cyng::param_factory("/monitor/tsdb", "/index.html"),
						cyng::param_factory("/monitor/lora", "/index.html"),

						cyng::param_factory("/task/csv", "/index.html"),
						cyng::param_factory("/task/tsdb", "/index.html"),
						cyng::param_factory("/task/plausibility", "/index.html")
					}))

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
		// The SSL context is required, and holds certificates
		//
		boost::asio::ssl::context ctx{ boost::asio::ssl::context::sslv23 };

		//
		//	connect to cluster
		//
		cyng::vector_t tmp_vec;
		cyng::tuple_t tmp_tpl;
		join_cluster(logger
			, mux
			, ctx
			, cyng::value_cast(cfg.get("cluster"), tmp_vec)
			, cyng::value_cast(cfg.get("server"), tmp_tpl));

		//
		//	wait for system signals
		//
		return wait(logger);
	}


	void join_cluster(cyng::logging::log_ptr logger
		, cyng::async::mux& mux
		, boost::asio::ssl::context& ctx
		, cyng::vector_t const& cfg_cls
		, cyng::tuple_t const& cfg_srv)
	{
		CYNG_LOG_TRACE(logger, "cluster redundancy: " << cfg_cls.size());

		auto dom = cyng::make_reader(cfg_srv);

		const boost::filesystem::path pwd = boost::filesystem::current_path();

		//	http::server build a string view
		static auto doc_root = cyng::value_cast(dom.get("document-root"), (pwd / "htdocs").string());
		auto address = cyng::value_cast<std::string>(dom.get("address"), "0.0.0.0");
		auto service = cyng::value_cast<std::string>(dom.get("service"), "8080");
		auto const host = cyng::make_address(address);
		auto const port = static_cast<unsigned short>(std::stoi(service));
		auto const timeout = cyng::numeric_cast<std::size_t>(dom.get("timeout"), 15u);
		auto const max_upload_size = cyng::numeric_cast<std::uint64_t>(dom.get("max-upload-size"), 1024u * 1024 * 10u);

		boost::system::error_code ec;
		if (boost::filesystem::exists(doc_root, ec)) {
			CYNG_LOG_INFO(logger, "document root: " << doc_root);
		}
		else {
			CYNG_LOG_FATAL(logger, "document root does not exists " << doc_root);
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

		//
		//	get user credentials
		//
		auth_dirs ad;
		init(dom.get("auth"), ad);
		for (auto const& dir : ad) {
			CYNG_LOG_INFO(logger, "restricted access to [" << dir.first << "]");
		}

		//
		//	get blacklisted addresses
		//
		const auto blacklist_str = cyng::vector_cast<std::string>(dom.get("blacklist"), "");
		CYNG_LOG_INFO(logger, blacklist_str.size() << " adresses are blacklisted");
		std::set<boost::asio::ip::address>	blacklist;
		for (auto const& a : blacklist_str) {
			auto r = blacklist.insert(boost::asio::ip::make_address(a));
			if (r.second) {
				CYNG_LOG_TRACE(logger, *r.first);
			}
			else {
				CYNG_LOG_WARNING(logger, "cannot insert " << a);
			}
		}

		//
		//	redirects
		//
		cyng::vector_t vec;
		auto const rv = cyng::value_cast(dom.get("redirect"), vec);
		auto const rs = cyng::to_param_map(rv);	// cyng::param_map_t
		CYNG_LOG_INFO(logger, rs.size() << " redirects configured");
		std::map<std::string, std::string> redirects;
		for (auto const& redirect : rs) {
			auto const target = cyng::value_cast<std::string>(redirect.second, "");
			CYNG_LOG_TRACE(logger, redirect.first
				<< " ==> "
				<< target);
			redirects.emplace(redirect.first, target);
		}

		static auto tls_pwd = cyng::value_cast<std::string>(dom.get("tls-pwd"), "test");
		auto tls_certificate_chain = cyng::value_cast<std::string>(dom.get("tls-certificate-chain"), "demo.cert");
		auto tls_private_key = cyng::value_cast<std::string>(dom.get("tls-private-key"), "priv.key");
		auto tls_dh = cyng::value_cast<std::string>(dom.get("tls-dh"), "demo.dh");

		//
		// This holds the self-signed certificate used by the server
		//
		if (load_server_certificate(ctx, logger, tls_pwd, tls_certificate_chain, tls_private_key, tls_dh)) {


			cyng::async::start_task_delayed<cluster>(mux
				, std::chrono::seconds(1)
				, logger
				, ctx
				, load_cluster_cfg(cfg_cls)
				, boost::asio::ip::tcp::endpoint{ host, port }
				, timeout
				, max_upload_size
				, doc_root
				, ad
				, blacklist
				, redirects);
		}
		else {
			CYNG_LOG_FATAL(logger, "loading server certificates failed");
		}

	}

	bool load_server_certificate(boost::asio::ssl::context& ctx
		, cyng::logging::log_ptr logger
		, std::string const& tls_pwd
		, std::string const& tls_certificate_chain
		, std::string const& tls_private_key
		, std::string const& tls_dh)
	{

		//
		//	generate files with (see https://www.adfinis-sygroup.ch/blog/de/openssl-x509-certificates/):
		//

		//	openssl genrsa -out solostec.com.key 4096
		//	openssl req -new -sha256 -key solostec.com.key -out solostec.com.csr
		//	openssl req -new -sha256 -nodes -newkey rsa:4096 -keyout solostec.com.key -out solostec.com.csr
		//	openssl req -x509 -sha256 -nodes -newkey rsa:4096 -keyout solostec.com.key -days 730 -out solostec.com.pem



		ctx.set_password_callback([&tls_pwd](std::size_t, boost::asio::ssl::context_base::password_purpose)	{
			return "test";
			//return tls_pwd;
		});

		ctx.set_options(
			boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			//boost::asio::ssl::context::no_sslv3 |
			boost::asio::ssl::context::single_dh_use);

		try {
			ctx.use_certificate_chain_file(tls_certificate_chain);
			ctx.use_private_key_file(tls_private_key, boost::asio::ssl::context::pem);
			ctx.use_tmp_dh_file(tls_dh);


// 			std::string const cert =
// 				"-----BEGIN CERTIFICATE-----\n"
// 				"MIIDaDCCAlCgAwIBAgIJAO8vBu8i8exWMA0GCSqGSIb3DQEBCwUAMEkxCzAJBgNV\n"
// 				"BAYTAlVTMQswCQYDVQQIDAJDQTEtMCsGA1UEBwwkTG9zIEFuZ2VsZXNPPUJlYXN0\n"
// 				"Q049d3d3LmV4YW1wbGUuY29tMB4XDTE3MDUwMzE4MzkxMloXDTQ0MDkxODE4Mzkx\n"
// 				"MlowSTELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAkNBMS0wKwYDVQQHDCRMb3MgQW5n\n"
// 				"ZWxlc089QmVhc3RDTj13d3cuZXhhbXBsZS5jb20wggEiMA0GCSqGSIb3DQEBAQUA\n"
// 				"A4IBDwAwggEKAoIBAQDJ7BRKFO8fqmsEXw8v9YOVXyrQVsVbjSSGEs4Vzs4cJgcF\n"
// 				"xqGitbnLIrOgiJpRAPLy5MNcAXE1strVGfdEf7xMYSZ/4wOrxUyVw/Ltgsft8m7b\n"
// 				"Fu8TsCzO6XrxpnVtWk506YZ7ToTa5UjHfBi2+pWTxbpN12UhiZNUcrRsqTFW+6fO\n"
// 				"9d7xm5wlaZG8cMdg0cO1bhkz45JSl3wWKIES7t3EfKePZbNlQ5hPy7Pd5JTmdGBp\n"
// 				"yY8anC8u4LPbmgW0/U31PH0rRVfGcBbZsAoQw5Tc5dnb6N2GEIbq3ehSfdDHGnrv\n"
// 				"enu2tOK9Qx6GEzXh3sekZkxcgh+NlIxCNxu//Dk9AgMBAAGjUzBRMB0GA1UdDgQW\n"
// 				"BBTZh0N9Ne1OD7GBGJYz4PNESHuXezAfBgNVHSMEGDAWgBTZh0N9Ne1OD7GBGJYz\n"
// 				"4PNESHuXezAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQCmTJVT\n"
// 				"LH5Cru1vXtzb3N9dyolcVH82xFVwPewArchgq+CEkajOU9bnzCqvhM4CryBb4cUs\n"
// 				"gqXWp85hAh55uBOqXb2yyESEleMCJEiVTwm/m26FdONvEGptsiCmF5Gxi0YRtn8N\n"
// 				"V+KhrQaAyLrLdPYI7TrwAOisq2I1cD0mt+xgwuv/654Rl3IhOMx+fKWKJ9qLAiaE\n"
// 				"fQyshjlPP9mYVxWOxqctUdQ8UnsUKKGEUcVrA08i1OAnVKlPFjKBvk+r7jpsTPcr\n"
// 				"9pWXTO9JrYMML7d+XRSZA1n3856OqZDX4403+9FnXCvfcLZLLKTBvwwFgEFGpzjK\n"
// 				"UEVbkhd5qstF6qWK\n"
// 				"-----END CERTIFICATE-----\n";
// 
// 			std::string const key =
// 				"-----BEGIN PRIVATE KEY-----\n"
// 				"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDJ7BRKFO8fqmsE\n"
// 				"Xw8v9YOVXyrQVsVbjSSGEs4Vzs4cJgcFxqGitbnLIrOgiJpRAPLy5MNcAXE1strV\n"
// 				"GfdEf7xMYSZ/4wOrxUyVw/Ltgsft8m7bFu8TsCzO6XrxpnVtWk506YZ7ToTa5UjH\n"
// 				"fBi2+pWTxbpN12UhiZNUcrRsqTFW+6fO9d7xm5wlaZG8cMdg0cO1bhkz45JSl3wW\n"
// 				"KIES7t3EfKePZbNlQ5hPy7Pd5JTmdGBpyY8anC8u4LPbmgW0/U31PH0rRVfGcBbZ\n"
// 				"sAoQw5Tc5dnb6N2GEIbq3ehSfdDHGnrvenu2tOK9Qx6GEzXh3sekZkxcgh+NlIxC\n"
// 				"Nxu//Dk9AgMBAAECggEBAK1gV8uETg4SdfE67f9v/5uyK0DYQH1ro4C7hNiUycTB\n"
// 				"oiYDd6YOA4m4MiQVJuuGtRR5+IR3eI1zFRMFSJs4UqYChNwqQGys7CVsKpplQOW+\n"
// 				"1BCqkH2HN/Ix5662Dv3mHJemLCKUON77IJKoq0/xuZ04mc9csykox6grFWB3pjXY\n"
// 				"OEn9U8pt5KNldWfpfAZ7xu9WfyvthGXlhfwKEetOuHfAQv7FF6s25UIEU6Hmnwp9\n"
// 				"VmYp2twfMGdztz/gfFjKOGxf92RG+FMSkyAPq/vhyB7oQWxa+vdBn6BSdsfn27Qs\n"
// 				"bTvXrGe4FYcbuw4WkAKTljZX7TUegkXiwFoSps0jegECgYEA7o5AcRTZVUmmSs8W\n"
// 				"PUHn89UEuDAMFVk7grG1bg8exLQSpugCykcqXt1WNrqB7x6nB+dbVANWNhSmhgCg\n"
// 				"VrV941vbx8ketqZ9YInSbGPWIU/tss3r8Yx2Ct3mQpvpGC6iGHzEc/NHJP8Efvh/\n"
// 				"CcUWmLjLGJYYeP5oNu5cncC3fXUCgYEA2LANATm0A6sFVGe3sSLO9un1brA4zlZE\n"
// 				"Hjd3KOZnMPt73B426qUOcw5B2wIS8GJsUES0P94pKg83oyzmoUV9vJpJLjHA4qmL\n"
// 				"CDAd6CjAmE5ea4dFdZwDDS8F9FntJMdPQJA9vq+JaeS+k7ds3+7oiNe+RUIHR1Sz\n"
// 				"VEAKh3Xw66kCgYB7KO/2Mchesu5qku2tZJhHF4QfP5cNcos511uO3bmJ3ln+16uR\n"
// 				"GRqz7Vu0V6f7dvzPJM/O2QYqV5D9f9dHzN2YgvU9+QSlUeFK9PyxPv3vJt/WP1//\n"
// 				"zf+nbpaRbwLxnCnNsKSQJFpnrE166/pSZfFbmZQpNlyeIuJU8czZGQTifQKBgHXe\n"
// 				"/pQGEZhVNab+bHwdFTxXdDzr+1qyrodJYLaM7uFES9InVXQ6qSuJO+WosSi2QXlA\n"
// 				"hlSfwwCwGnHXAPYFWSp5Owm34tbpp0mi8wHQ+UNgjhgsE2qwnTBUvgZ3zHpPORtD\n"
// 				"23KZBkTmO40bIEyIJ1IZGdWO32q79nkEBTY+v/lRAoGBAI1rbouFYPBrTYQ9kcjt\n"
// 				"1yfu4JF5MvO9JrHQ9tOwkqDmNCWx9xWXbgydsn/eFtuUMULWsG3lNjfst/Esb8ch\n"
// 				"k5cZd6pdJZa4/vhEwrYYSuEjMCnRb0lUsm7TsHxQrUd6Fi/mUuFU/haC0o0chLq7\n"
// 				"pVOUFq5mW8p0zbtfHbjkgxyF\n"
// 				"-----END PRIVATE KEY-----\n";
// 
// 			std::string const dh =
// 				"-----BEGIN DH PARAMETERS-----\n"
// 				"MIIBCAKCAQEArzQc5mpm0Fs8yahDeySj31JZlwEphUdZ9StM2D8+Fo7TMduGtSi+\n"
// 				"/HRWVwHcTFAgrxVdm+dl474mOUqqaz4MpzIb6+6OVfWHbQJmXPepZKyu4LgUPvY/\n"
// 				"4q3/iDMjIS0fLOu/bLuObwU5ccZmDgfhmz1GanRlTQOiYRty3FiOATWZBRh6uv4u\n"
// 				"tff4A9Bm3V9tLx9S6djq31w31Gl7OQhryodW28kc16t9TvO1BzcV3HjRPwpe701X\n"
// 				"oEEZdnZWANkkpR/m/pfgdmGPU66S2sXMHgsliViQWpDCYeehrvFRHEdR9NV+XJfC\n"
// 				"QMUk26jPTIVTLfXmmwU0u8vUkpR7LQKkwwIBAg==\n"
// 				"-----END DH PARAMETERS-----\n";
// 			ctx.use_certificate_chain(
// 				boost::asio::buffer(cert.data(), cert.size()));
// 
// 			ctx.use_private_key(
// 				boost::asio::buffer(key.data(), key.size()),
// 				boost::asio::ssl::context::file_format::pem);
// 
// 			ctx.use_tmp_dh(
// 				boost::asio::buffer(dh.data(), dh.size()));

		}
		catch (std::exception const& ex) {
			CYNG_LOG_FATAL(logger, ex.what());
			return false;

		}
		return true;
	}

}
