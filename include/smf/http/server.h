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
//#include <boost/beast/websocket.hpp>
//#include <boost/beast/version.hpp>
//#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
//#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
//#include <boost/make_unique.hpp>
//#include <boost/optional.hpp>
//#include <algorithm>
//#include <cstdlib>
//#include <functional>
//#include <iostream>
//#include <memory>
//#include <string>
//#include <thread>
//#include <vector>

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
			server(boost::asio::io_context& ioc, cyng::logger, std::string);

			bool start(boost::asio::ip::tcp::endpoint ep);

		private:
			void run();
			void do_accept();
			void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);
			void close();

		private:
			boost::asio::io_context& ioc_;
			boost::asio::ip::tcp::acceptor acceptor_;
			cyng::logger logger_;
			std::string const doc_root_;

			/**
			 *	set to true when the server is listening for new connections
			 */
			std::atomic<bool>	is_running_;

		};

	}	//	http
}

#endif
