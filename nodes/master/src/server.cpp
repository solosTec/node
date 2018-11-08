/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 


#include "server.h"
#include "db.h"
#include "connection.h"
#include <cyng/dom/reader.h>

namespace node 
{
	server::server(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, boost::uuids::uuid tag
		, std::string account
		, std::string pwd
		, int monitor	//	cluster
		, cyng::tuple_t cfg_session)
	: mux_(mux)
		, logger_(logger)
		, tag_(tag)
		, account_(account)
		, pwd_(pwd)
		, monitor_(monitor)
		, global_configuration_(0)
		, stat_dir_()
		, max_messages_(1000u)
		, acceptor_(mux.get_io_service())
#if (BOOST_VERSION < 106600)
		, socket_(io_ctx_)
#endif
		, db_()
		, uidgen_()
	{
		//
		//	read session configuration
		//
		auto dom = cyng::make_reader(cfg_session);

		//
		//	set global configuration bitmask
		//
		set_connection_auto_login(global_configuration_, cyng::value_cast(dom.get("auto-login"), false));
		set_connection_auto_enabled(global_configuration_, cyng::value_cast(dom.get("auto-enabled"), true));
		set_connection_superseed(global_configuration_, cyng::value_cast(dom.get("supersede"), true));
		set_generate_time_series(global_configuration_, cyng::value_cast(dom.get("generate-time-series"), false));
		set_catch_meters(global_configuration_, cyng::value_cast(dom.get("catch-meters"), false));

		CYNG_LOG_TRACE(logger_, "global configuration bitmask: " << global_configuration_.load());

		const boost::filesystem::path tmp = boost::filesystem::temp_directory_path();
		stat_dir_ = cyng::value_cast(dom.get("stat-dir"), tmp.string());
		CYNG_LOG_INFO(logger_, "store statistics data at " << stat_dir_);

		max_messages_ = cyng::value_cast<std::uint64_t>(dom.get("ax-messages"), max_messages_);
		CYNG_LOG_INFO(logger_, "store max. " << max_messages_ << " messages");

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
			//	initialize database
			//	
			CYNG_LOG_TRACE(logger_, "init database");
			init(logger_
				, db_
				, tag_
				, acceptor_.local_endpoint()
				, global_configuration_.load()
				, stat_dir_
				, max_messages_);

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
					, tag_
					, db_
					, account_
					, pwd_
					, tag
					, monitor_ // cluster watchdog
					, global_configuration_
					, stat_dir_)->start();

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
			<< tag_
			;
		insert_msg(db_
			, cyng::logging::severity::LEVEL_FATAL
			, ss.str()
			, tag_);

		CYNG_LOG_WARNING(logger_, "close server");

		//
		//	terminate all sessions
		//
		mux_.post("node::watchdog", 0, cyng::tuple_factory(tag_));

		// The server is stopped by cancelling all outstanding asynchronous
        // operations. Once all operations have finished the io_context::run()
        // call will exit.
        acceptor_.close();

	}
	

}



