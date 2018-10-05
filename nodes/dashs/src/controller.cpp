/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/cluster.h"
#include <NODE_project_info.h>
//#include <smf/https/srv/auth.h>

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

#if BOOST_OS_WINDOWS
#include <cyng/scm/service.hpp>
#endif
#if BOOST_OS_LINUX
#include "../../write_pid.h"
#endif
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

namespace node 
{
	//
	//	forward declaration(s):
	//
	bool start(cyng::async::mux&, cyng::logging::log_ptr, cyng::object);
	bool wait(cyng::logging::log_ptr logger);
	void join_cluster(cyng::async::mux&, cyng::logging::log_ptr, cyng::vector_t const&, cyng::tuple_t const&);
	bool load_server_certificate(boost::asio::ssl::context& ctx
		, cyng::logging::log_ptr
		, std::string const& tls_pwd
		, std::string const& tls_certificate_chain
		, std::string const& tls_private_key
		, std::string const& tls_dh);

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
					//	start application
					//
					cyng::vector_t vec;
					vec = cyng::value_cast(config, vec);

					if (vec.empty())
					{
						std::cerr
							<< "use option -D to generate a configuration file"
							<< std::endl;
						shutdown = true;
					}
					else
					{

						//
						//	initialize logger
						//
#if BOOST_OS_LINUX
						auto logger = cyng::logging::make_sys_logger("dashs", true);
#else
						const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
						auto dom = cyng::make_reader(vec[0]);
						const boost::filesystem::path log_dir = cyng::value_cast(dom.get("log-dir"), tmp.string());

						auto logger = (console)
							? cyng::logging::make_console_logger(mux.get_io_service(), "dashs")
							: cyng::logging::make_file_logger(mux.get_io_service(), (log_dir / "smf-dashs.log"))
							;
#ifdef _DEBUG
						if (!console) {
							std::cout << "log file see: " << (log_dir / "smf-dashs.log") << std::endl;
						}
#endif
#endif

						CYNG_LOG_TRACE(logger, cyng::io::to_str(config));
						CYNG_LOG_INFO(logger, "pool size: " << this->pool_size_);

						//
						//	start
						//
						shutdown = start(mux, logger, vec[0]);

						//
						//	print uptime
						//
						const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tp_start);
						CYNG_LOG_INFO(logger, "uptime " << cyng::io::to_str(cyng::make_object(duration)));
					}
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
			boost::uuids::random_generator uidgen;

			//
			//	reconnect to master on different times
			//
			boost::random::mt19937 rng_;
            rng_.seed(std::time(nullptr));
			boost::random::uniform_int_distribution<int> monitor_dist(10, 120);

			const auto conf = cyng::vector_factory({
				cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
                    , cyng::param_factory("log-level", "INFO")
					, cyng::param_factory("tag", uidgen())
					, cyng::param_factory("generated", std::chrono::system_clock::now())
					, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))

					, cyng::param_factory("server", cyng::tuple_factory(
						cyng::param_factory("address", "0.0.0.0"),
						cyng::param_factory("service", "8443"),	//	default is 443
						cyng::param_factory("document-root", (pwd / "nodes" / "dash" / "htdocs").string()),
						cyng::param_factory("sub-protocols", cyng::vector_factory({ "SMF", "LoRa" })),
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
								cyng::param_factory("realm", "Root Areay"),
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
						//185.244.25.187
						cyng::param_factory("blacklist", cyng::vector_factory({
							//	https://bl.isx.fr/raw
							cyng::make_address("185.244.25.187"),	//	KV Solutions B.V. scans for "login.cgi"
							cyng::make_address("139.219.100.104")	//	ISP Microsoft (China) Co. Ltd. - 2018-07-31T21:14
							})),	//	blacklist
						cyng::param_factory("redirect", cyng::vector_factory({
							cyng::param_factory("/", "/index.html")
						}))

					))

					, cyng::param_factory("cluster", cyng::vector_factory({ cyng::tuple_factory(
						cyng::param_factory("host", "127.0.0.1"),
						cyng::param_factory("service", "7701"),
						cyng::param_factory("account", "root"),
						cyng::param_factory("pwd", NODE_PWD),
						cyng::param_factory("salt", NODE_SALT),
						cyng::param_factory("monitor", monitor_dist(rng_)),	//	seconds
						cyng::param_factory("auto-config", false),	//	client security
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

	//int controller::generate_x509(std::string const& c
	//	, std::string const& l
	//	, std::string const& o
	//	, std::string const& cn)
	//{
	//	//::OPENSSL_init_ssl(0, NULL);

	//	cyng::crypto::EVP_KEY_ptr pkey(cyng::crypto::generate_key(), ::EVP_PKEY_free);
	//	if (!pkey)
	//	{
	//		return EXIT_FAILURE;
	//	}
	//	cyng::crypto::X509_ptr x509(cyng::crypto::generate_x509(pkey.get()
	//		, 31536000L
	//		, c.c_str()
	//		//, l.c_str()
	//		, o.c_str()
	//		, cn.c_str()), ::X509_free);
	//	if (!x509)
	//	{
	//		return EXIT_FAILURE;
	//	}

	//	std::cout << "Writing key and certificate to disk..." << std::endl;
	//	bool ret = cyng::crypto::write_to_disk(pkey.get(), x509.get());
	//	return (ret)
	//		? EXIT_SUCCESS
	//		: EXIT_FAILURE;
	//}

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
			::OutputDebugString("An instance of the [e350] service is already running.");
			break;
		case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
			//
			//	The service process could not connect to the service controller.
			//	Typical error message, when running in console mode.
			//
			::OutputDebugString("***Error 1063: The [e350] service process could not connect to the service controller.");
			std::cerr
				<< "***Error 1063: The [e350] service could not connect to the service controller."
				<< std::endl
				;
			break;
		case ERROR_SERVICE_NOT_IN_EXE:
			//	The executable program that this service is configured to run in does not implement the service.
			::OutputDebugString("The [e350] service is configured to run in does not implement the service.");
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

		boost::uuids::random_generator uidgen;
		const auto tag = cyng::value_cast<boost::uuids::uuid>(dom.get("tag"), uidgen());

		//
		//	apply severity threshold
		//
		logger->set_severity(cyng::logging::to_severity(cyng::value_cast<std::string>(dom.get("log-level"), "INFO")));

#if BOOST_OS_LINUX
		const boost::filesystem::path log_dir = cyng::value_cast<std::string>(dom.get("log-dir"), ".");
		write_pid(log_dir, tag);
#endif

		//
		//	connect to cluster
		//
		cyng::vector_t tmp_vec;
		cyng::tuple_t tmp_tpl;
		join_cluster(mux, logger, cyng::value_cast(dom.get("cluster"), tmp_vec), cyng::value_cast(dom.get("server"), tmp_tpl));

		//
		//	wait for system signals
		//
		const bool shutdown = wait(logger);

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

	void join_cluster(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
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
		const auto port = static_cast<unsigned short>(std::stoi(service));

		//
		//	get subprotocols
		//
		//const auto sub_protocols = cyng::vector_cast<std::string>(dom.get("sub-protocols"), "");

		CYNG_LOG_INFO(logger, "document root: " << doc_root);
		CYNG_LOG_INFO(logger, "address: " << address);
		CYNG_LOG_INFO(logger, "service: " << service);

		//
		// The SSL context is required, and holds certificates
		//
		boost::asio::ssl::context ctx{ boost::asio::ssl::context::sslv23 };

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
			, doc_root);
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
		//	generate files with:
		//	openssl dhparam -out dh.pem 2048
		//	openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 10000 -out cert.pem -subj "//C=CH\ST=LU\L=Lucerne\O=solosTec\CN=www.example.com"
		//


		ctx.set_password_callback([&tls_pwd](std::size_t, boost::asio::ssl::context_base::password_purpose)	{
			return tls_pwd;
		});

		ctx.set_options(
			boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::single_dh_use);

		try {
			ctx.use_certificate_chain_file(tls_certificate_chain);
			ctx.use_private_key_file(tls_private_key, boost::asio::ssl::context::pem);
			ctx.use_tmp_dh_file(tls_dh);
		}
		catch (std::exception const& ex) {
			CYNG_LOG_FATAL(logger, ex.what());
			return false;

		}
		return true;
	}

}
