/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_HTTP_SERVER_H
#define SMF_HTTP_SERVER_H

#include <cyng/log/logger.h>


#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>

namespace smf {
	namespace http
	{
		/**
		 * Modelled after 
		 * https://github.com/boostorg/beast/blob/develop/example/advanced/server/advanced_server.cpp
		 */
		class server
		{
		public:
			using accept_cb = std::function<void(boost::asio::ip::tcp::socket&&)>;

		public:
			server(boost::asio::io_context& ioc, cyng::logger, accept_cb);

			bool start(boost::asio::ip::tcp::endpoint ep);
			void stop();

		private:
			void run();
			void do_accept();
			void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);

		public:
			boost::asio::io_context& ioc_;

		private:
			boost::asio::ip::tcp::acceptor acceptor_;
			cyng::logger logger_;
			accept_cb cb_;

			/**
			 *	set to true when the server is listening for new connections
			 */
			std::atomic<bool>	is_running_;

		};

	}	//	http
}

#endif
