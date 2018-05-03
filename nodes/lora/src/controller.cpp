/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/cluster.h"
#include <NODE_project_info.h>
#include <smf/https/srv/auth.h>
#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/async/task/task_builder.hpp>
#include <cyng/async/signal_handler.h>
#include <cyng/factory/set_factory.h>
#include <cyng/io/serializer.h>
#include <cyng/json.h>
#include <cyng/dom/reader.h>
#include <cyng/dom/tree_walker.h>
#include <cyng/vector_cast.hpp>
#include <cyng/crypto/x509.h>

#if BOOST_OS_WINDOWS
#include <cyng/scm/service.hpp>
#endif
#if BOOST_OS_LINUX
#include <cyng/sys/process.h>
#endif
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/random.hpp>

namespace node 
{
	//
	//	forward declaration(s):
	//
	bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::object);
	bool wait(cyng::logging::log_ptr logger);
	std::size_t join_cluster(cyng::async::mux&, cyng::logging::log_ptr, boost::uuids::uuid, cyng::vector_t const&, cyng::tuple_t const&);
	void load_server_certificate(boost::asio::ssl::context& ctx
		, std::string const& pwd
		, std::string const& dh
		, std::string const&  private_key
		, std::string const& certificate_chain);

	controller::controller(unsigned int pool_size, std::string const& json_path)
	: pool_size_(pool_size)
	, json_path_(json_path)
	{}
	
	int controller::run(bool console)
	{
		//
		//	to calculate uptime
		//
		const std::chrono::system_clock::time_point tp_start = std::chrono::system_clock::now();
		//
		//	controller loop
		//
		try
		{
			//
			//	controller loop
			//
			bool shutdown{ false };
			while (!shutdown)
			{
				//
				//	establish I/O context
				//
				cyng::async::mux mux{ this->pool_size_ };

				//
				//	read configuration file
				//
				cyng::object config = cyng::json::read_file(json_path_);

				if (config)
				{
					//
					//	initialize logger
					//
#if BOOST_OS_LINUX
					auto logger = cyng::logging::make_sys_logger("LoRa", true);
#else
					auto logger = cyng::logging::make_console_logger(mux.get_io_service(), "LoRa");
#endif

					CYNG_LOG_TRACE(logger, cyng::io::to_str(config));

					//
					//	start application
					//
					cyng::vector_t vec;
					vec = cyng::value_cast(config, vec);
					BOOST_ASSERT_MSG(!vec.empty(), "invalid configuration");
					shutdown = vec.empty()
						? true
						: start(mux, logger, vec[0]);

					//
					//	print uptime
					//
					const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tp_start);
					CYNG_LOG_INFO(logger, "uptime " << cyng::io::to_str(cyng::make_object(duration)));
				}
				else
				{
					// 	CYNG_LOG_FATAL(logger, "no configuration data");
					std::cout
						<< "use option -D to generate a configuration file"
						<< std::endl;

					//
					//	no configuration found
					//
					shutdown = true;
				}

				//
				//	shutdown scheduler
				//
				mux.shutdown();
			}

			return EXIT_SUCCESS;
		}
		catch (std::exception& ex)
		{
			std::cerr
				<< ex.what()
				<< std::endl;
		}

		return EXIT_FAILURE;
	}

	int controller::create_config() const
	{
		std::fstream fout(json_path_, std::ios::trunc | std::ios::out);
		if (fout.is_open())
		{
			std::cout
				<< "write to file "
				<< json_path_
				<< std::endl;
			//
			//	get default values
			//
			const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
			const boost::filesystem::path pwd = boost::filesystem::current_path();
			boost::uuids::random_generator rgen;

			boost::random::mt19937 rng_;
			rng_.seed(std::time(0));
			boost::random::uniform_int_distribution<int> monitor_dist(10, 120);

			const auto conf = cyng::vector_factory({
				cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
					, cyng::param_factory("log-level", "INFO")
					, cyng::param_factory("tag", rgen())
					, cyng::param_factory("generated", std::chrono::system_clock::now())

					, cyng::param_factory("server", cyng::tuple_factory(
						cyng::param_factory("address", "0.0.0.0"),
						cyng::param_factory("service", "8443"),
						cyng::param_factory("document-root", (pwd / "htdocs").string()),
						cyng::param_factory("sub-protocols", cyng::vector_factory({ "SMF", "LoRa" })),
						cyng::param_factory("tls-pwd", "test"),
						cyng::param_factory("tls-certificate-chain", "demo.cert"),
						cyng::param_factory("tls-private-kay", "priv.key"),
						cyng::param_factory("tls-dh", "demo.dh"),	//	diffie-hellman
						cyng::param_factory("auth", cyng::vector_factory({
							//	directory: /
							//	authType:
							//	user:
							//	pwd:
							cyng::tuple_factory(
								cyng::param_factory("directory", "/"),
								cyng::param_factory("authType", "Basic"),
								cyng::param_factory("realm", "Root Area"),
								cyng::param_factory("name", "auth@example.com"),
								cyng::param_factory("pwd", "secret")
							),
							cyng::tuple_factory(
								cyng::param_factory("directory", "/temp"),
								cyng::param_factory("authType", "Basic"),
								cyng::param_factory("realm", "Restricted Content"),
								cyng::param_factory("name", "auth@example.com"),
								cyng::param_factory("pwd", "secret")
							) } )),	//	auth

						//185.244.25.187
						cyng::param_factory("blacklist", cyng::vector_factory({
							cyng::make_address("185.244.25.187")	//	KV Solutions B.V. scans for login.cgi
							})),	//	blacklist

						cyng::param_factory("redirect", cyng::vector_factory({
							cyng::param_factory("/", "/index.html")
							}))	//	redirect
						))	//	server
					, cyng::param_factory("cluster", cyng::vector_factory({ cyng::tuple_factory(
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "7701"),
						cyng::param_factory("account", "root"),
						cyng::param_factory("pwd", NODE_PWD),
						cyng::param_factory("salt", NODE_SALT),
						cyng::param_factory("monitor", monitor_dist(rng_)),	//	seconds
						cyng::param_factory("group", 0)	//	customer ID
					) }))
				)
				});

			cyng::json::write(std::cout, cyng::make_object(conf));
			std::cout << std::endl;
			cyng::json::write(fout, cyng::make_object(conf));
			return EXIT_SUCCESS;
		}
		else
		{
			std::cerr
				<< "unable to open file "
				<< json_path_
				<< std::endl;

		}
		return EXIT_FAILURE;
	}

	int controller::generate_x509(std::string const& c
		, std::string const& l
		, std::string const& o
		, std::string const& cn)
	{
        //::OPENSSL_init_ssl(0, NULL);

		cyng::crypto::EVP_KEY_ptr pkey(cyng::crypto::generate_key(), ::EVP_PKEY_free);
		if (!pkey)
		{
			return EXIT_FAILURE;
		}
		cyng::crypto::X509_ptr x509(cyng::crypto::generate_x509(pkey.get()
			, 31536000L
			, c.c_str()
			//, l.c_str()
			, o.c_str()
			, cn.c_str()), ::X509_free);
		if (!x509)
		{
			return EXIT_FAILURE;
		}

		std::cout << "Writing key and certificate to disk..." << std::endl;
		bool ret = cyng::crypto::write_to_disk(pkey.get(), x509.get());
		return (ret)
			? EXIT_SUCCESS
			: EXIT_FAILURE;
	}

#if BOOST_OS_WINDOWS
	int controller::run_as_service(controller&& ctrl, std::string const& srv_name)
	{
		//
		//	define service type
		//
		typedef service< controller >	service_type;

		//
		//	create service
		//
		::OutputDebugString(srv_name.c_str());
		service_type srv(std::move(ctrl), srv_name);

		//
		//	starts dispatcher and calls service main() function 
		//
		const DWORD r = srv.run();
		switch (r)
		{
		case ERROR_SERVICE_ALREADY_RUNNING:
			//	An instance of the service is already running.
			::OutputDebugString("An instance of the [LoRa] service is already running.");
			break;
		case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
			//
			//	The service process could not connect to the service controller.
			//	Typical error message, when running in console mode.
			//
			::OutputDebugString("***Error 1063: The [LoRa] service process could not connect to the service controller.");
			std::cerr
				<< "***Error 1063: The [LoRa] service could not connect to the service controller."
				<< std::endl
				;
			break;
		case ERROR_SERVICE_NOT_IN_EXE:
			//	The executable program that this service is configured to run in does not implement the service.
			::OutputDebugString("The [LoRa] service is configured to run in does not implement the service.");
			break;
		default:
		{
			std::stringstream ss;
			ss
				<< '['
				<< srv_name
				<< "] service dispatcher stopped "
				<< r;
			const std::string msg = ss.str();
			::OutputDebugString(msg.c_str());
		}
		break;
		}


		return EXIT_SUCCESS;
	}

	void controller::control_handler(DWORD sig)
	{
		//	forward signal to shutdown manager
		cyng::forward_signal(sig);
	}

#endif

	bool start(cyng::async::mux& mux, cyng::logging::log_ptr logger, cyng::object cfg)
	{
		CYNG_LOG_TRACE(logger, cyng::dom_counter(cfg) << " configuration nodes found");
		auto dom = cyng::make_reader(cfg);

		boost::uuids::random_generator rgen;
		const auto tag = cyng::value_cast<boost::uuids::uuid>(dom.get("tag"), rgen());

		//
		//	apply severity threshold
		//
		logger->set_severity(cyng::logging::to_severity(cyng::value_cast<std::string>(dom.get("log-level"), "INFO")));

#if BOOST_OS_LINUX
        const boost::filesystem::path log_dir = cyng::value_cast<std::string>(dom.get("log-dir"), ".");
        const auto pid = (log_dir / boost::uuids::to_string(tag)).replace_extension(".pid").string();
        std::fstream fout(pid, std::ios::trunc | std::ios::out);
        if (fout.is_open())
        {
            CYNG_LOG_INFO(logger, "write PID file: " << pid);
            fout << cyng::sys::get_process_id();
            fout.close();
        }
        else
        {
            CYNG_LOG_ERROR(logger, "could not write PID file: " << pid);
        }
#endif

		//
		//	connect to cluster
		//
		cyng::vector_t tmp_vec;
		cyng::tuple_t tmp_tpl;
		const auto tsk = join_cluster(mux, logger, tag, cyng::value_cast(dom.get("cluster"), tmp_vec), cyng::value_cast(dom.get("server"), tmp_tpl));

		//
		//	wait for system signals
		//
		const bool shutdown = wait(logger);

		//
		//	stop cluster
		//
		CYNG_LOG_INFO(logger, "stop cluster");
		mux.stop(tsk);

		//
		//	stop all tasks
		//
		CYNG_LOG_INFO(logger, "stop all tasks");
		mux.stop();

		return shutdown;
	}

	bool wait(cyng::logging::log_ptr logger)
	{
		//
		//	wait for system signals
		//
		bool shutdown = false;
		cyng::signal_mgr signal;
		switch (signal.wait())
		{
#if BOOST_OS_WINDOWS
		case CTRL_BREAK_EVENT:
#else
		case SIGHUP:
#endif
			CYNG_LOG_INFO(logger, "SIGHUP received");
			break;
		default:
			CYNG_LOG_WARNING(logger, "SIGINT received");
			shutdown = true;
			break;
		}
		return shutdown;
	}

	std::size_t join_cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, cyng::vector_t const& cfg_cls
		, cyng::tuple_t const& cfg_srv)
	{
		CYNG_LOG_TRACE(logger, "cluster redundancy: " << cfg_cls.size());

		auto dom = cyng::make_reader(cfg_srv);

		const boost::filesystem::path wd = boost::filesystem::current_path();

		//	http::server build a string view
		static auto doc_root = cyng::value_cast(dom.get("document-root"), (wd / "htdocs").string());
		auto address = cyng::value_cast<std::string>(dom.get("address"), "0.0.0.0");
		auto service = cyng::value_cast<std::string>(dom.get("service"), "8443");
		auto const host = cyng::make_address(address);
		const auto port = static_cast<unsigned short>(std::stoi(service));
		static auto pwd = cyng::value_cast<std::string>(dom.get("tls-pwd"), "test");
		auto certificate_chain = cyng::value_cast<std::string>(dom.get("tls-certificate-chain"), "cert.pem");
		auto private_key = cyng::value_cast<std::string>(dom.get("tls-private-kay"), "key.pem");
		auto dh = cyng::value_cast<std::string>(dom.get("tls-dh"), "dh.pem");

		//
		//	get subprotocols
		//
		const auto sub_protocols = cyng::vector_cast<std::string>(dom.get("sub-protocols"), "");

		//
		//	get user credentials
		//
		auth_dirs ad;
		init(dom.get("auth"), ad);

		CYNG_LOG_INFO(logger, "document root: " << doc_root);
		CYNG_LOG_INFO(logger, "address: " << address);
		CYNG_LOG_INFO(logger, "service: " << service);
		CYNG_LOG_INFO(logger, sub_protocols.size() << " subprotocols");
		CYNG_LOG_INFO(logger, ad.size() << " user credentials");

		// The SSL context is required, and holds certificates
		static boost::asio::ssl::context ctx{ boost::asio::ssl::context::sslv23 };

		// This holds the self-signed certificate used by the server
		load_server_certificate(ctx, pwd, dh, private_key, certificate_chain);

		auto r = cyng::async::start_task_delayed<cluster>(mux
			, std::chrono::seconds(1)
			, logger
			, tag
			, load_cluster_cfg(cfg_cls)
			, boost::asio::ip::tcp::endpoint{ host, port }
			, doc_root
			, sub_protocols
			, ctx);

		if (r.second)	return r.first;

		CYNG_LOG_FATAL(logger, "could not start cluster");
		return cyng::async::NO_TASK;
	}

	/*  Load a signed certificate into the ssl context, and configure
		the context for use with a server.

		For this to work with the browser or operating system, it is
		necessary to import the "Beast Test CA" certificate into
		the local certificate store, browser, or operating system
		depending on your environment Please see the documentation
		accompanying the Beast certificate for more details.
	 */
	void load_server_certificate(boost::asio::ssl::context& ctx
		, std::string const& pwd
		, std::string const& dh
		, std::string const& private_key
		, std::string const& certificate_chain)
	{
		/*
		The certificate was generated from CMD.EXE on Windows 10 using:

		winpty openssl dhparam -out dh.pem 2048
		winpty openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 10000 -out cert.pem -subj "//C=CH\ST=LU\L=Lucerne\O=solosTec\CN=www.solostec.com"
		*/

		//std::string const cert =
		//	"-----BEGIN CERTIFICATE-----\n"
		//	"MIIC1jCCAb6gAwIBAgIJAIA++QcqOgT5MA0GCSqGSIb3DQEBCwUAMAAwHhcNMTgw\n"
		//	"MjI4MDAwNjM2WhcNNDUwNzE2MDAwNjM2WjAAMIIBIjANBgkqhkiG9w0BAQEFAAOC\n"
		//	"AQ8AMIIBCgKCAQEArcSoBvNg6iNLfryyeanjeM8akBMLxJPM69Ar3K+gtacuvsvc\n"
		//	"JyPaZ0+HXYZyXZ4dI8/AuVZID6ESKq5HxNYtzuHuH7oFNm0Yd7nlM/nJdyA78vBb\n"
		//	"12htq8Me4rg01AaIYrxczwm9GHFl4qy/CHpHJ6TA7mIaa6qXtcoFOMQh1yKDcsQi\n"
		//	"Cn2E6r9N/lwsTorr+HRqsnaeLyLekhWlYxInWCGXe8o8un7vhNXWnDno/0RIUID5\n"
		//	"rOKtDBZrFbngurxpDdcMgvnig4SHoaa8sq5ul+xc9x4SVRecWPtNCDvA/K+4jgkm\n"
		//	"93RByScCQ7m2ROeE1LpNwDCHUH10emb7WdKcUQIDAQABo1MwUTAdBgNVHQ4EFgQU\n"
		//	"Bi2HmS31plRxUZTJcW2WvtircKowHwYDVR0jBBgwFoAUBi2HmS31plRxUZTJcW2W\n"
		//	"vtircKowDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAeQAA1ZDv\n"
		//	"S/Tsozk3FsITgU+/+KuxKBza3dYsS1psWqTnYb2HsxSoZUoho3RxA4BHDtJJ299I\n"
		//	"yG8xyb9NSrSOBoEjoWL8z8X7INf6ipLYtjG1Bcmalkc53TlYkR6nuE8dPvqb9N4R\n"
		//	"5Pq2IOWF2zFssIW06MncfuRo2ERFZcK4bmEz1/AaIlcPlilAHb7ciCf0tRokUafn\n"
		//	"gyPoHFxKOjslCTBT/WmY72VrS10Ypk0DqcE7IcgT/wYfY64Kn69jsiHmwlm4dqTw\n"
		//	"7bz9ob96UrCOMzlDj23vvuuyGKkUdzXYDob2VwqrqjNpuB/+cMElEE6w8i3OQ2Cj\n"
		//	"KvnMBplhJIP+IA==\n"
		//	"-----END CERTIFICATE-----\n";

		//std::string const key =
		//	"-----BEGIN PRIVATE KEY-----\n"
		//	"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCtxKgG82DqI0t+\n"
		//	"vLJ5qeN4zxqQEwvEk8zr0Cvcr6C1py6+y9wnI9pnT4ddhnJdnh0jz8C5VkgPoRIq\n"
		//	"rkfE1i3O4e4fugU2bRh3ueUz+cl3IDvy8FvXaG2rwx7iuDTUBohivFzPCb0YcWXi\n"
		//	"rL8IekcnpMDuYhprqpe1ygU4xCHXIoNyxCIKfYTqv03+XCxOiuv4dGqydp4vIt6S\n"
		//	"FaVjEidYIZd7yjy6fu+E1dacOej/REhQgPms4q0MFmsVueC6vGkN1wyC+eKDhIeh\n"
		//	"pryyrm6X7Fz3HhJVF5xY+00IO8D8r7iOCSb3dEHJJwJDubZE54TUuk3AMIdQfXR6\n"
		//	"ZvtZ0pxRAgMBAAECggEBAIsPiSRe2t0lN8KKAg5pTdgdbXWFOHKtkV3Z73AhwOv+\n"
		//	"ieM4w8sy3xK0S3EmKhoPceR52xK3IN4ZGa+8X0T/3hLlLaqINKm0rtMJmop4yKij\n"
		//	"zDYD8ou1T6cYdHwdzHEtdTIG6gLqGUEZZt77PbnsGUt5hsh/DAPDtrtNm9Ys56QA\n"
		//	"8jUb0wLPpgf5qRXoDo9VUf9Y0+OSlW6ojjwWm1+Bc6AADdM3KZ4hbQLq84gqOXs9\n"
		//	"eETM//k3p+Qfx+6rWjx4d7Y4UgDAx/b9JHy21KLiNi0I7klmu9PehXMBQBpsWT3w\n"
		//	"jLZB5x7uNsjTjTf8BFDrwLmhKRcRxo0QrrWeND7M400CgYEA1VP1iPqixOqvzGFN\n"
		//	"PDZKdjt3AoOo/eH6avT5IOfDdnBzu7yL9nsoKetKEVf3iODdgtaYgW8FnkxK9hu3\n"
		//	"iJXzItZR4lOwM4U5bypJAD+6n1LGN/l7otmOBFnRtixgmCfsv2z0x7dX+uO5as1f\n"
		//	"YsK3LrXazynUOA53d9hELdp16EMCgYEA0IbuThg47pPkQSGUe6Tc3Sz2rLx18lZQ\n"
		//	"pyy5c/Vij7nQFex8hOynEMlvT7vPDk8XQWgOeGWT/dtSK681orQhp+J7LosPDE6J\n"
		//	"1IvUxS7IjsoSMtx8mw7pT5ewaP4XDW1WLLu8HB8IzDvHP3BBUtrTH5i9jrQkFLqs\n"
		//	"eR6yl2PqOdsCgYEAjk4xrqyzQ/TiTM5jvVTiGzjTzOOTKblDWXINdnvkke+15HiE\n"
		//	"TWoegsgoYqVxxOdsHMmWdlFfSBfQsZgPuJd+17BsczQsiFHI3HUyuW3JylpnTBOq\n"
		//	"/BlweUqJcKLt1NJdRd0i9M9Da2PZ3nsdtD38ALbjPerDXJmZ7GJiKMxgdw0CgYAb\n"
		//	"oLT0Hdt1KJ0GUBenJhmpKCrqifGqkOsQqylLBsjvN/Qs429AAUbFP5sC2mQ9hhcT\n"
		//	"sGCybOrlqGhDp2wYyXroDma5rOzqeYFjar9e/KrP2E/+8x2DQb+BrxxNXNTbD5Bq\n"
		//	"TtlGdIoq3QSyEAJnotx0BD2hKZbaND1jssCAtFk1HwKBgCCv/6OYrWCEe0GE6QGY\n"
		//	"+vqzOF2jojlx681bj3qn37+j1WYDD38aTk2UjdDsYegJFqgHBlj/+vMLs+XiStTK\n"
		//	"gVJXyEMIJuZn4gxeiH5v8l3iQiaGvJCwkUIveYXrj0ca7AwT9JBPKkid+PDAgPXh\n"
		//	"btU9uQ/qzH2kDyaZg4FGTTY5\n"
		//	"-----END PRIVATE KEY-----\n";

		//std::string const dh =
		//	"-----BEGIN DH PARAMETERS-----\n"
		//	"MIIBCAKCAQEA9VTzDc1fJLRTnNMyF04ka323hlMg5mENKUZwQJkmPTyFas9XlMNW\n"
		//	"ZUFlQR2lMK2qCLb5ijqI5GGiOu+RMu2NpDKtj7Puwhys3Kg6NhyNGOgx+p2mTNX8\n"
		//	"rBgbs1GAOuImrbV03H/JhsVV6kVO5yySPc0A4o7PxPYkOORqI1Uaujf0xeGAQS5W\n"
		//	"cnlhXrxj5bVhKjo6lc+YWBAWjw2P4K0BlVvisV2MIvh0p2XA9Y4GvlICmRiOtJbS\n"
		//	"5vwprVwf697etoEqQy2x++ggmJo5F+MIuEZ4TRX5e++xbeeFx5SUO/HIySWM7e9L\n"
		//	"UvTUfvTrEETqPkQda0tBd4uA4H2lg7pwkwIBAg==\n"
		//	"-----END DH PARAMETERS-----\n";

		ctx.set_password_callback(
			[&pwd](std::size_t,
				boost::asio::ssl::context_base::password_purpose)
		{
			return pwd;
			//return "test";
		});

		ctx.set_options(
			boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::single_dh_use);

		//ctx.use_certificate_chain(
		//	boost::asio::buffer(cert.data(), cert.size()));

		//ctx.use_private_key(
		//	boost::asio::buffer(key.data(), key.size()),
		//	boost::asio::ssl::context::file_format::pem);

		//ctx.use_tmp_dh(
		//	boost::asio::buffer(dh.data(), dh.size()));

		ctx.use_certificate_chain_file(certificate_chain);
		ctx.use_private_key_file(private_key, boost::asio::ssl::context::pem);
		ctx.use_tmp_dh_file(dh);
	}

}
