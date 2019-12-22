/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_CONNECTION_H
#define NODE_SEGW_CONNECTION_H

#include <NODE_project_info.h>
#include <smf/sml/bus/serializer.h>

#include <cyng/object.h>
#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/vm/context.h>
#include <array>
#include <memory>

namespace node
{
	/**
	 * SEGW Custom interface
	 */
	class session;
	class cache;
	class storage;
	class connection : public std::enable_shared_from_this<connection>
	{
	public:
		connection(connection const&) = delete;
		connection& operator=(connection const&) = delete;
		explicit connection(boost::asio::ip::tcp::socket&&
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, cache& cfg
			, storage& db
			, std::string const& account
			, std::string const& pwd
			, bool accept_all);

		/**
		 * Start the first asynchronous operation for the connection.
		 */
		void start();

		/**
		 * Stop all asynchronous operations associated with the connection.
		 */
		void stop();

	private:
		session * get_session();
		session const* get_session() const;

		/**
		 * Perform an asynchronous read operation.
		 */
		void do_read();

		/**
		 * create a reference of this object on stack.
		 */
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
		std::array<char, NODE::PREFERRED_BUFFER_SIZE> buffer_;

		/**
		 * Implements the session logic
		 */
		cyng::object session_;

		/**
		 * wrapper class to serialize and send
		 * data and code.
		 */
		sml::serializer		serializer_;

	};
}

#endif