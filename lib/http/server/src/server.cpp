/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#include <smf/http/srv/server.h>
#include <smf/http/srv/session.h>

namespace node
{
	namespace http
	{

		server::server(boost::asio::io_context& ioc,
			boost::asio::ip::tcp::endpoint endpoint,
			std::string const& doc_root)
			: acceptor_(ioc)
			, socket_(ioc)
			, doc_root_(doc_root)
		{
			boost::system::error_code ec;

			// Open the acceptor
			acceptor_.open(endpoint.protocol(), ec);
			if (ec)
			{
				BOOST_ASSERT_MSG(!ec, "OPEN");
				return;
			}

			// Bind to the server address
			acceptor_.bind(endpoint, ec);
			if (ec)
			{
				BOOST_ASSERT_MSG(!ec, "BIND");
				return;
			}

			// Start listening for connections
			acceptor_.listen(
				boost::asio::socket_base::max_listen_connections, ec);
			if (ec)
			{
				BOOST_ASSERT_MSG(!ec, "LISTEN");
				return;
			}
		}

		// Start accepting incoming connections
		void server::run()
		{
			if (!acceptor_.is_open())
			{
				return;
			}
			do_accept();
		}

		void server::do_accept()
		{
			acceptor_.async_accept(
				socket_,
				std::bind(
					&server::on_accept,
					this,
					//shared_from_this(),
					std::placeholders::_1));
		}

		void server::on_accept(boost::system::error_code ec)
		{
			if (ec)
			{
				BOOST_ASSERT_MSG(!ec, "ACCEPT");
				return;
			}
			else
			{
				// Create the http_session and run it
				std::make_shared<http_session>(
					std::move(socket_),
					doc_root_)->run();
			}

			// Accept another connection
			do_accept();
		}

		void server::close()
		{
			acceptor_.cancel();
			acceptor_.close();

		}
	}
}