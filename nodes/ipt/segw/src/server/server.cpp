/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */


#include "server.h"
#include "../cache.h"
#include "connection.h"

#include <cyng/object.h>

namespace node
{
	server::server(cyng::async::mux& mux
		, cyng::logging::log_ptr logger
		, cache& cfg
		, storage& db
		, std::string account
		, std::string pwd
		, bool accept_all)
	: mux_(mux)
		, logger_(logger)
		, cache_(cfg)
		, storage_(db)
		, account_(account)
		, pwd_(pwd)
		, accept_all_(accept_all)
		, acceptor_(mux.get_io_service())
#if (BOOST_VERSION < 106600)
		, socket_(io_ctx_)
#endif
	{}

	void server::run(std::string const& address, std::string const& service)
	{
		//
		// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
		//
		try {
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

			do_accept();

			//
			//	update status word: service interface available
			//
			cache_.set_status_word(sml::STATUS_BIT_SERVICE_IF_AVAILABLE, true);
		}
		catch (std::exception const& ex) {

			CYNG_LOG_FATAL(logger_, ex.what());
			cache_.set_status_word(sml::STATUS_BIT_SERVICE_IF_AVAILABLE, false);

		}
	}

	void server::do_accept()
	{
#if (BOOST_VERSION >= 106600)
		acceptor_.async_accept(
			[this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
			// Check whether the server was stopped by a signal before this
			// completion handler had a chance to run.
			if (!acceptor_.is_open())
			{
				return;
			}

			if (!ec)
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
					, cache_
					, storage_
					, account_
					, pwd_
					, accept_all_)->start();

				do_accept();
			}

		});
#else
		acceptor_.async_accept(socket_, [=](boost::system::error_code const& ec) {
			// Check whether the server was stopped by a signal before this
			// completion handler had a chance to run.
			if (acceptor_.is_open() && !ec) {

				CYNG_LOG_TRACE(logger_, "accept " << socket_.remote_endpoint());
				//
				//	There is no connection manager or list of open connections. 
				//	Connections are managed by there own and are controlled
				//	by a maintenance task.
				//
				//std::make_shared<connection>(std::move(socket), mux_, logger_, db_)->start();
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
		// The server is stopped by cancelling all outstanding asynchronous
		// operations. Once all operations have finished the io_context::run()
		// call will exit.
		acceptor_.close();
	}


}



