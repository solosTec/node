/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_SRV_SERVER_H
#define NODE_LIB_HTTP_SRV_SERVER_H

#include <memory>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>

namespace node
{
	namespace http
	{
		class server : public std::enable_shared_from_this<server>
		{
			boost::asio::ip::tcp::acceptor acceptor_;
			boost::asio::ip::tcp::socket socket_;
			std::string const& doc_root_;

		public:
			server(boost::asio::io_context& ioc,
				boost::asio::ip::tcp::endpoint endpoint,
				std::string const& doc_root);

			// Start accepting incoming connections
			void run();
			void do_accept();

			/**
			 * Close acceptor
			 */
			void close();

		private:
			void on_accept(boost::system::error_code ec);
		};
	}
}

#endif
