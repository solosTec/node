/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MASTER_CONNECTION_H
#define NODE_MASTER_CONNECTION_H

#include "session.h"
#include <smf/cluster/serializer.h>
#include <NODE_project_info.h>
#include <cyng/log.h>
#include <array>
#include <memory>

namespace node 
{
	/**
	 * Cluster member connection
	 */
	class connection : public std::enable_shared_from_this<connection>
	{
	public:
		connection(connection const&) = delete;
		connection& operator=(connection const&) = delete;
		
		/**
		 * Construct a connection with the specified socket 
		 */
		explicit connection(boost::asio::ip::tcp::socket&&
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cyng::store::db&
			, std::string const& account
			, std::string const& pwd
			, std::chrono::seconds const& monitor);
		
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

		session* get_session();
		session const* get_session() const;

		void push_session(cyng::context& ctx);

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
		//session	session_;
		cyng::object session_;

		/**
		 * wrapper class to serialize and send
		 * data and code.
		 */
		serializer		serializer_;

	};
}

#endif



