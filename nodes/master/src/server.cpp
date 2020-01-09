/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 


#include "server.h"
//#include "db.h"
#include "connection.h"
#include <cyng/dom/reader.h>

namespace node 
{
	server::server(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, std::string country_code
		, std::string language_code
		, std::string account
		, std::string pwd
		, int monitor	//	cluster
		, cyng::tuple_t cfg_session)
	: mux_(mux)
		, logger_(logger)
		, account_(account)
		, pwd_(pwd)
		//, monitor_(monitor)
		, acceptor_(mux.get_io_service())
#if (BOOST_VERSION < 106600)
		, socket_(io_ctx_)
#endif
		, db_()
		, cache_(db_, tag)
		, uidgen_()
	{
		//
		//	read session configuration
		//
		auto dom = cyng::make_reader(cfg_session);

		//
		//	create cache tables
		//
		create_tables(logger, db_, tag);

		//
		//	initialize global configuration data
		//
		const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
		auto stat_dir = cyng::value_cast(dom.get("stat-dir"), tmp.string());
		CYNG_LOG_INFO(logger_, "store statistics data at " << stat_dir);

		CYNG_LOG_TRACE(logger_, "country code: " << country_code);

		auto max_messages = cyng::value_cast<std::uint64_t>(dom.get("max-messages"), 1000);
		CYNG_LOG_INFO(logger_, "store max. " << max_messages << " messages");

		auto max_events = cyng::value_cast<std::uint64_t>(dom.get("max-events"), 2000);
		CYNG_LOG_INFO(logger_, "store max. " << max_events << " events");

		cache_.init(country_code
			, language_code
			, stat_dir
			, max_messages
			, max_events
			, std::chrono::seconds(monitor));
	}
	
	void server::run(std::string const& address, std::string const& service)
	{
		CYNG_LOG_TRACE(logger_, "resolve address "
			<< address
			<< ':'
			<< service);

		try {
			// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
			boost::asio::ip::tcp::resolver resolver(mux_.get_io_service());
#if (BOOST_VERSION >= 106600)
			boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(address, service).begin();
#else
			boost::asio::ip::tcp::resolver::query query(address, service);
			boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
#endif
			acceptor_.open(endpoint.protocol());
			acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			acceptor_.bind(endpoint);
			acceptor_.listen();

			//
			//	create master node record
			//	
			cache_.create_master_record(acceptor_.local_endpoint());

			//
			//	start reading
			//
			do_accept();
		}
		catch (std::exception const& ex) {
			CYNG_LOG_FATAL(logger_, "resolve address "
				<< address
				<< ':'
				<< service
				<< " failed: "
				<< ex.what());
		}
	}
	
	void server::do_accept()
	{
#if (BOOST_VERSION >= 106600)
		acceptor_.async_accept([this] ( boost::system::error_code ec, boost::asio::ip::tcp::socket socket ) {
			// Check whether the server was stopped by a signal before this
			// completion handler had a chance to run.
			if ( !acceptor_.is_open() ) 
			{
				return;
			}

			if ( !ec ) 
			{
				const auto tag = uidgen_();

				CYNG_LOG_TRACE(logger_, "accept "
					<< socket.remote_endpoint()
					<< " - "
					<< tag);

				//
				//	There is no connection manager or list of open connections. 
				//	Connections are managed by there own and are controlled
				//	by a maintenance task.
				//
				
				std::make_shared<connection>(std::move(socket)
					, mux_
					, logger_
					, cache_
					, account_
					, pwd_
					, tag)->start();

				do_accept();
			}

		} );
#else
		acceptor_.async_accept(socket_,	[=](boost::system::error_code const& ec) {
			// Check whether the server was stopped by a signal before this
			// completion handler had a chance to run.
			if (acceptor_.is_open() && !ec) {

				CYNG_LOG_TRACE(logger_, "accept " << socket_.remote_endpoint());
				//
				//	There is no connection manager or list of open connections. 
				//	Connections are managed by there own and are controlled
				//	by a maintenance task.
				//
				std::make_shared<connection>(std::move(socket), mux_, logger_, db_)->start();
				do_accept();
			}

			else {
				//on_error(ec);
				//	shutdown server
			}

		});
#endif

	}
	
	void server::close()
	{
		//
		//	shutdown message
		//
		std::stringstream ss;
		ss
			<< "shutdown cluster master "
			<< cache_.get_tag()
			;
		cache_.insert_msg(cyng::logging::severity::LEVEL_FATAL
			, ss.str()
			, cache_.get_tag());

		CYNG_LOG_WARNING(logger_, "close server");

		// The server is stopped by cancelling all outstanding asynchronous
		// operations. Once all operations have finished the io_context::run()
		// call will exit.
		acceptor_.close();

		//
		//	terminate all sessions
		//
		mux_.post("node::watchdog", 0, cyng::tuple_factory(cache_.get_tag()));

		//
		//	wait for all watchdog tasks to terminate
		//

		std::atomic< bool >	complete{ false };
		std::size_t msec{ 10 };
		while (!complete) {
			mux_.size("node::watchdog", [&](std::size_t count) {

				//
				//	this runs in another thread than the while loop()
				//
				complete = (count == 0);
				if (!complete) {
					
					CYNG_LOG_WARNING(logger_, "waiting for " << count << " watchdog task(s) -  time left " << (1000 - msec) << " msec");

					msec += 20;
					complete = (msec > 1000);	// last exit
				}
			});

			//
			//	outside thread
			//
			std::this_thread::sleep_for(std::chrono::milliseconds(msec));
		}

		CYNG_LOG_INFO(logger_, "closing master server complete");

	}
}



