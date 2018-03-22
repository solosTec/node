/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_SRV_SERVER_H
#define NODE_LIB_HTTP_SRV_SERVER_H

#include "connections.h"
#include <cyng/store/db.h>
#include <smf/cluster/bus.h>

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
		class server
		{
		public:
			server(cyng::logging::log_ptr
				, boost::asio::io_context& ioc
				, boost::asio::ip::tcp::endpoint endpoint
				, std::string const& doc_root
				, node::bus::shared_type
				, cyng::store::db&);

			// Start accepting incoming connections
			bool run();
			void do_accept();

			/**
			 * Close acceptor
			 */
			void close();

			/**
			 * send data to websocket
			 */
			//void run_on_ws(boost::uuids::uuid, cyng::vector_t&&);
			bool send_msg(boost::uuids::uuid, std::string const&);

			void add_channel(boost::uuids::uuid tag, std::string const& channel);
			//void process_event(std::string const& channel, cyng::vector_t&&);
			void process_event(std::string const& channel, std::string const&);

		private:
			void on_accept(boost::system::error_code ec);
            bool bind(boost::asio::ip::tcp::endpoint, std::size_t);

		private:
			boost::asio::ip::tcp::acceptor acceptor_;
			boost::asio::ip::tcp::socket socket_;
			cyng::logging::log_ptr logger_;
			std::string const& doc_root_;
			bus::shared_type bus_;
			cyng::store::db& cache_;

			/**
			 * The connection manager which owns all live connections.
			 */
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

			/**
			 * generate unique session tags
			 */
			boost::uuids::random_generator rgn_;

		};
	}
}

#endif
