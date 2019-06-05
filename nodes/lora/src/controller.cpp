/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "controller.h"
#include "tasks/cluster.h"
#include <NODE_project_info.h>
#include <smf/http/srv/auth.h>
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
#include <cyng/set_cast.h>
 //#include <cyng/crypto/x509.h>
#include <cyng/rnd.h>

#if BOOST_OS_WINDOWS
#include <cyng/scm/service.hpp>
#endif
#if BOOST_OS_LINUX
#include "../../write_pid.h"
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
	std::size_t join_cluster(cyng::async::mux&, cyng::logging::log_ptr, boost::uuids::uuid, bool, cyng::vector_t const&, cyng::tuple_t const&);
	void load_server_certificate(boost::asio::ssl::context& ctx
		, cyng::logging::log_ptr logger
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
						auto logger = cyng::logging::make_sys_logger("smf:LoRa", true);
#else
						const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
						auto dom = cyng::make_reader(vec[0]);
						const boost::filesystem::path log_dir = cyng::value_cast(dom.get("log-dir"), tmp.string());

						auto logger = (console)
							? cyng::logging::make_console_logger(mux.get_io_service(), "smf:LoRa")
							: cyng::logging::make_file_logger(mux.get_io_service(), (log_dir / "smf:LoRa.log"))
							;
#ifdef _DEBUG
						if (!console) {
							std::cout << "log file see: " << (log_dir / "smf:LoRa.log") << std::endl;
						}
#endif
#endif

						CYNG_LOG_TRACE(logger, cyng::io::to_str(config));
						CYNG_LOG_INFO(logger, "pool size: " << this->pool_size_);

						//
						//	start application
						//
						shutdown = start(mux, logger, vec[0]);

						//
						//	print uptime
						//
						const auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tp_start);
						CYNG_LOG_INFO(logger, "smf:LoRa uptime " << cyng::io::to_str(cyng::make_object(duration)));
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

			cyng::crypto::rnd_num<int> rng(10, 120);

			const auto conf = cyng::vector_factory({
				cyng::tuple_factory(cyng::param_factory("log-dir", tmp.string())
					, cyng::param_factory("log-level", "INFO")
					, cyng::param_factory("tag", uidgen())
					, cyng::param_factory("generated", std::chrono::system_clock::now())
					, cyng::param_factory("version", cyng::version(NODE_VERSION_MAJOR, NODE_VERSION_MINOR))
					, cyng::param_factory("keep-xml-files", false)	//	store all incoming data as XML file

					, cyng::param_factory("server", cyng::tuple_factory(
						cyng::param_factory("address", "0.0.0.0"),
						cyng::param_factory("service", "8443"),
						cyng::param_factory("document-root", (pwd / "htdocs").string()),
						cyng::param_factory("tls-pwd", "test"),
						cyng::param_factory("tls-certificate-chain", "fullchain.cert"),
						cyng::param_factory("tls-private-key", "privkey.key"),
						cyng::param_factory("tls-dh", "dh4096.dh"),	//	diffie-hellman
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
							cyng::make_address("185.244.25.187"),	//	KV Solutions B.V. scans for login.cgi
							cyng::make_address("139.219.100.104"),	//	ISP Microsoft (China) Co. Ltd. - 2018-07-31T21:14
							cyng::make_address("194.147.32.109"),	//	Udovikhin Evgenii - 2019-02-01 15:23:08.13699453
							cyng::make_address("184.105.247.196")	//	Hurricane Electric LLC, scan-15.shadowserver.org - 2019-02-01 16:13:13.81835055
							//cyng::make_address("178.38.88.91")
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
						cyng::param_factory("monitor", rng()),	//	seconds
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
 //       //::OPENSSL_init_ssl(0, NULL);

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

		boost::uuids::random_generator uidgen;
		auto const cluster_tag = cyng::value_cast<boost::uuids::uuid>(dom.get("tag"), uidgen());
		CYNG_LOG_TRACE(logger, "cluster tag: " << cluster_tag);

		auto const keep_xml_file = cyng::value_cast(dom.get("keep-xml-files"), false);
		if (keep_xml_file) {
			CYNG_LOG_TRACE(logger, "incoming XML files are stored");
		}
		
		//
		//	apply severity threshold
		//
		logger->set_severity(cyng::logging::to_severity(cyng::value_cast<std::string>(dom.get("log-level"), "INFO")));

#if BOOST_OS_LINUX
        const boost::filesystem::path log_dir = cyng::value_cast<std::string>(dom.get("log-dir"), ".");
        write_pid(log_dir, cluster_tag);
#endif

		//
		//	connect to cluster
		//
		cyng::vector_t tmp_vec;
		cyng::tuple_t tmp_tpl;
		const auto tsk = join_cluster(mux
			, logger
			, cluster_tag
			, keep_xml_file
			, cyng::value_cast(dom.get("cluster"), tmp_vec)
			, cyng::value_cast(dom.get("server"), tmp_tpl));

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
		, boost::uuids::uuid cluster_tag
		, bool keep_xml_files
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


		CYNG_LOG_INFO(logger, "document root: " << doc_root);
		CYNG_LOG_INFO(logger, "address: " << address);
		CYNG_LOG_INFO(logger, "service: " << service);

		//
		//	get user credentials
		//
		auth_dirs ad;
		init(dom.get("auth"), ad);
		//CYNG_LOG_INFO(logger, ad.size() << " user credentials");
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

		// The SSL context is required, and holds certificates
		static boost::asio::ssl::context ctx{ boost::asio::ssl::context::sslv23 };

		//
		//	get SSL configuration
		//
		static auto tls_pwd = cyng::value_cast<std::string>(dom.get("tls-pwd"), "test");
		auto tls_certificate_chain = cyng::value_cast<std::string>(dom.get("tls-certificate-chain"), "cert.pem");
		auto tls_private_key = cyng::value_cast<std::string>(dom.get("tls-private-key"), "key.pem");
		auto tls_dh = cyng::value_cast<std::string>(dom.get("tls-dh"), "dh.pem");

		CYNG_LOG_TRACE(logger, "tls-certificate-chain: " << tls_certificate_chain);
		CYNG_LOG_TRACE(logger, "tls-private-key: " << tls_private_key);
		CYNG_LOG_TRACE(logger, "tls-dh: " << tls_dh);

		// This holds the self-signed certificate used by the server
		load_server_certificate(ctx, logger, tls_pwd, tls_certificate_chain, tls_private_key, tls_dh);

		auto r = cyng::async::start_task_delayed<cluster>(mux
			, std::chrono::seconds(1)
			, logger
			, ctx
			, cluster_tag
			, keep_xml_files
			, load_cluster_cfg(cfg_cls)
			, boost::asio::ip::tcp::endpoint{ host, port }
			, doc_root
			, ad
			, blacklist
			, redirects);

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
		, cyng::logging::log_ptr logger
		, std::string const& pwd
		, std::string const& tls_certificate_chain
		, std::string const& tls_private_key
		, std::string const& tls_dh)
	{
		//
		//	generate files with:
		//	openssl dhparam -out dh.pem 2048
		//	openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 10000 -out cert.pem -subj "//C=CH\ST=LU\L=Lucerne\O=solosTec\CN=www.example.com"
		//
		ctx.set_password_callback(
			[&pwd](std::size_t,
				boost::asio::ssl::context_base::password_purpose)
		{
			return pwd;
		});

		ctx.set_options(
			boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::single_dh_use);

		try {
			ctx.use_certificate_chain_file(tls_certificate_chain);
			CYNG_LOG_INFO(logger, tls_certificate_chain << " successfull loaded");

			ctx.use_private_key_file(tls_private_key, boost::asio::ssl::context::pem);
			CYNG_LOG_INFO(logger, tls_private_key << " successfull loaded");

			ctx.use_tmp_dh_file(tls_dh);
			CYNG_LOG_INFO(logger, tls_dh << " successfull loaded");
		}
		catch (std::exception const& ex) {
			CYNG_LOG_FATAL(logger, ex.what());
		}
	}

}
