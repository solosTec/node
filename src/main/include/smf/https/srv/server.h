/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTPS_SRV_SERVER_H
#define NODE_LIB_HTTPS_SRV_SERVER_H

//#include <smf/https/srv/https.h>
#include "connections.h"
#include <smf/http/srv/auth.h>
#include <cyng/compatibility/async.h>
#include <cyng/vm/controller_fwd.h>
#include <atomic>

namespace node
{
	namespace https
	{
		/**
		 * Accepts incoming HTTPS connections and launches the sessions
		 */
		class server
		{
		public:
			server(cyng::logging::log_ptr
				, boost::asio::io_context& ioc
				, boost::asio::ssl::context& ctx
				, boost::asio::ip::tcp::endpoint endpoint
				, std::string const& doc_root
				, auth_dirs const& ad
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

		private:
			cyng::logging::log_ptr logger_;
			boost::asio::ssl::context& ctx_;
			boost::asio::ip::tcp::acceptor acceptor_;
			boost::asio::ip::tcp::socket socket_;

			/**
			 * The connection manager which owns all live connections.
			 */
			connections connection_manager_;

			/**
			* all blacklisted addresses
			 */
			const std::set<boost::asio::ip::address>	blacklist_;

			/**
			 *	set to true when the server is listening for new connections
			 */
			std::atomic<bool>	is_listening_;

			/**
			 *	Synchronisation objects for proper shutdown
			 */
			cyng::async::condition_variable shutdown_complete_;
			cyng::async::mutex mutex_;

		};
	}
}

#endif