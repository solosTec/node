/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_GATEWAY_CONNECTION_H
#define NODE_IPT_GATEWAY_CONNECTION_H

#include <NODE_project_info.h>
#include <smf/sml/bus/serializer.h>
#include <smf/sml/status.h>
#include <smf/ipt/config.h>

#include <cyng/object.h>
#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/vm/context.h>
#include <array>
#include <memory>

namespace node
{
	namespace sml
	{
		/**
		* Gateway configuration session
		*/
		class session;
		class connection : public std::enable_shared_from_this<connection>
		{
		public:
			connection(connection const&) = delete;
			connection& operator=(connection const&) = delete;
			explicit connection(boost::asio::ip::tcp::socket&&
				, cyng::async::mux& mux
				, cyng::logging::log_ptr logger
				, status& status_word
				, cyng::store::db& config_db
				, node::ipt::master_config_t const& cfg
				, std::string const& account
				, std::string const& pwd
				, std::string manufacturer
				, std::string model
				, std::uint32_t serial
				, cyng::mac48 mac);

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
			serializer		serializer_;

		};
	}
}

#endif