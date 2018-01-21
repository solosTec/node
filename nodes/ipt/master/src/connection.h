/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_MASTER_CONNECTION_H
#define NODE_IPT_MASTER_CONNECTION_H

#include "session.h"
#include <NODE_project_info.h>
#include <smf/ipt/serializer.h>
#include <smf/cluster/bus.h>
#include <cyng/log.h>
#include <array>
#include <memory>

namespace node 
{
	namespace ipt
	{
		/**
		 * Cluster member connection
		 */
		class server;
		class connection : public std::enable_shared_from_this<connection>
		{
			friend class server;
		public:
			using shared_type = std::shared_ptr<connection>;

		public:
			connection(connection const&) = delete;
			connection& operator=(connection const&) = delete;

			/**
			 * Construct a connection with the specified socket
			 */
			explicit connection(boost::asio::ip::tcp::socket&&
				, cyng::async::mux& mux
				, cyng::logging::log_ptr logger
				, bus::shared_type
				, boost::uuids::uuid
				, scramble_key const& sk
				, std::uint16_t watchdog
				, std::chrono::seconds const& timeout);

			/**
			 * Start the first asynchronous operation for the connection.
			 */
			void start();

			/**
			 * Stop all asynchronous operations associated with the connection.
			 */
			void stop();

		private:
			/**
			 * Perform an asynchronous read operation.
			 */
			void do_read();

			/**
			 * Perform an asynchronous write operation.
			 */
			void do_write();

		private:
			/**
			 * connection socket
			 */
			boost::asio::ip::tcp::socket socket_;

			/**
			 * The logger instance
			 */
			cyng::logging::log_ptr logger_;

			/**
			 * Buffer for incoming data.
			 */
			std::array<char, NODE_PREFERRED_BUFFER_SIZE> buffer_;

			/**
			 * Implements the session logic
			 */
			session	session_;

			/**
			 * wrapper class to serialize and send
			 * data and code.
			 */
			serializer		serializer_;

		};
	}
}

#endif



