/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_SRV_SERVER_H
#define NODE_LIB_HTTP_SRV_SERVER_H

#include "connections.h"
#ifdef NODE_SSL_INSTALLED
#include <smf/http/srv/auth.h>
#endif

#include <cyng/log.h>
#include <cyng/compatibility/async.h>

#include <atomic>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <boost/uuid/random_generator.hpp>

namespace node
{
	namespace http
	{
		/**
		 * Accepts incoming HTTP connections and launches the sessions
		 */
		class server
		{
		public:
			server(cyng::logging::log_ptr
				, boost::asio::io_context& ioc
				, boost::asio::ip::tcp::endpoint endpoint
				, std::string const& doc_root
#ifdef NODE_SSL_INSTALLED
				, auth_dirs const& ad
#endif
				, std::set<boost::asio::ip::address> const& blacklist
				, cyng::controller& vm);

			// Start accepting incoming connections
			bool run();
			void do_accept();

			/**
			 * Close acceptor
			 */
			void close();

			/**
			 * access connection manager
			 */
			connection_manager_interface& get_cm();

		private:
			void on_accept(boost::system::error_code ec);
            bool bind(boost::asio::ip::tcp::endpoint, std::size_t);

		private:
			cyng::logging::log_ptr logger_;
			boost::asio::ip::tcp::acceptor acceptor_;
			boost::asio::ip::tcp::socket socket_;
			const std::set<boost::asio::ip::address> blacklist_;

			/**
			 * The connection manager which owns all live connections.
			 */
			connections connection_manager_;

			/**
			 *	set to true when the server is listening for new connections
			 */
			std::atomic<bool>	is_listening_;

			/**
			 *	Synchronisation objects for proper shutdown
			 */
			cyng::async::condition_variable shutdown_complete_;
			cyng::async::mutex mutex_;

			/**
			 * generate unique session tags
			 */
			boost::uuids::random_generator uidgen_;

		};
	}
}

#endif
