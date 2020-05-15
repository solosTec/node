/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTPS_SRV_SERVER_H
#define NODE_LIB_HTTPS_SRV_SERVER_H

#include "connections.h"

#include <smf/http/srv/auth.h>
#include <cyng/compatibility/async.h>
#include <cyng/vm/controller_fwd.h>

#include <atomic>
#include <memory>

namespace node
{
	namespace https
	{
		/**
		 * Accepts incoming HTTPS connections and launches the sessions
		 */
		class server //: public std::enable_shared_from_this<server>
		{
		public:
			server(cyng::logging::log_ptr
				, boost::asio::io_context& ioc
				, boost::asio::ssl::context& ctx
				, boost::asio::ip::tcp::endpoint endpoint
                , std::size_t timeout
				, std::uint64_t max_upload_size
				, std::string const& doc_root
				, std::string const& nickname
				, auth_dirs const& ad
				, std::set<boost::asio::ip::address> const& blacklist
				, std::map<std::string, std::string> const& redirects
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
#if (BOOST_BEAST_VERSION < 248)
			void on_accept(boost::system::error_code ec);
#else
			void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);
#endif
			bool bind(boost::asio::ip::tcp::endpoint ep, std::size_t retries);
			
		private:
			cyng::logging::log_ptr logger_;
			boost::asio::ssl::context& ctx_;
			boost::asio::ip::tcp::acceptor acceptor_;
#if (BOOST_BEAST_VERSION < 248)
			boost::asio::ip::tcp::socket socket_;
#else
			boost::asio::io_context& ioc_;
#endif

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
