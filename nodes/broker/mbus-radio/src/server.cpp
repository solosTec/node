/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#include "server.h"
#include "session.h"

namespace node
{
	server::server(cyng::io_service_t& ios
		, cyng::logging::log_ptr logger
		, boost::asio::ip::tcp::endpoint ep)
	: acceptor_(ios, ep)
		, logger_(logger)
	{
		CYNG_LOG_INFO(logger_, "configured local endpoint " << ep);
	}

	void server::run()
	{
		do_accept();
	}

	void server::close()
	{
		acceptor_.cancel();
		acceptor_.close();
	}
	 
	void server::do_accept()
	{
		acceptor_.async_accept(
			[this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
			{
				if (!ec)
				{
					CYNG_LOG_TRACE(logger_, "accept " << socket.remote_endpoint());
					std::make_shared<session>(std::move(socket), logger_)->start();
				}

				do_accept();
			});
	}

}
