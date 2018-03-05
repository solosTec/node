/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTPS_SRV_SERVER_H
#define NODE_LIB_HTTPS_SRV_SERVER_H

#include <memory>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <atomic>

namespace node
{
	namespace https
	{
		// Accepts incoming connections and launches the sessions
		class server : public std::enable_shared_from_this<server>
		{
		public:
			server(boost::asio::io_context& ioc,
				boost::asio::ssl::context& ctx,
				boost::asio::ip::tcp::endpoint endpoint,
				std::string const& doc_root);
			// Start accepting incoming connections
			bool run();
			void do_accept();

			/**
			 * Close acceptor
			 */
			void close();

		private:
			void on_accept(boost::system::error_code ec);

		private:
			boost::asio::ssl::context& ctx_;
			boost::asio::ip::tcp::acceptor acceptor_;
			boost::asio::ip::tcp::socket socket_;
			std::string const& doc_root_;

			/**
			 *	set to true when the server is listening for new connections
			 */
			std::atomic<bool>	is_listening_;

		};
	}
}

#endif