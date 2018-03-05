/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HTTP_LISTENER_H
#define NODE_HTTP_LISTENER_H

#include <cyng/compatibility/io_service.h>
#include "connections.h"
#include "mail_config.h"
#include "auth.h"
#include <cyng/log.h>
#include <cyng/compatibility/async.h>
#include <memory>
#include <atomic>

namespace node 
{
	// Accepts incoming connections and launches the sessions
	class listener : public std::enable_shared_from_this<listener>
	{
		
	public:
		listener(cyng::logging::log_ptr,
			cyng::io_service_t& ioc,
			boost::asio::ip::tcp::endpoint endpoint,
			std::string const& doc_root,
			mail_config const&,
			auth_dirs const&);

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
		cyng::logging::log_ptr logger_;
		boost::asio::ip::tcp::acceptor acceptor_;
		boost::asio::ip::tcp::socket socket_;
		const std::string doc_root_;
		const mail_config mail_config_;
		const auth_dirs auth_;
		/// The connection manager which owns all live connections.
		connection_manager connection_manager_;

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

#endif
