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
		, connection_open_timeout_(12)
		, connection_close_timeout_(12)
		, connection_auto_login_(false)
		, connection_auto_enabled_(true)
		, connection_superseed_(true)
		, acceptor_(mux.get_io_service())
#if (BOOST_VERSION < 106600)
		, socket_(io_ctx_)
#endif
		, db_()
		, rgn_()
	{
		//
		//	read session configuration
		//
		auto dom = cyng::make_reader(cfg_session);
		connection_open_timeout_ = std::chrono::seconds(cyng::value_cast(dom.get("connection-open-timeout"), 12));
		connection_open_timeout_ = std::chrono::seconds(cyng::value_cast(dom.get("connection-close-timeout"), 12));
		connection_auto_login_ = cyng::value_cast(dom.get("auto-login"), connection_auto_login_);
		connection_auto_enabled_ = cyng::value_cast(dom.get("auto-enabled"), connection_auto_enabled_);
		connection_superseed_ = cyng::value_cast(dom.get("supersede"), connection_superseed_);

	}
	
	void server::run(std::string const& address, std::string const& service)
	{
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
			, connection_open_timeout_
			, connection_close_timeout_
			, connection_auto_login_
			, connection_auto_enabled_
			, connection_superseed_);

		do_accept();
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
				CYNG_LOG_TRACE(logger_, "accept " << socket.remote_endpoint());
				
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
					, rgn_()
					, monitor_ // cluster watchdog
					, connection_open_timeout_
					, connection_close_timeout_
					, connection_auto_login_
					, connection_auto_enabled_
					, connection_superseed_)->start();

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
		insert_msg(db_
			, cyng::logging::severity::LEVEL_FATAL
			, "server closed"
			, tag_);

		CYNG_LOG_FATAL(logger_, "close server");

		// The server is stopped by cancelling all outstanding asynchronous
        // operations. Once all operations have finished the io_context::run()
        // call will exit.
        acceptor_.close();

	}
	

}



