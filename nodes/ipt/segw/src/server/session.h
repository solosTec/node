/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_SESSION_H
#define NODE_SEGW_SESSION_H

#include <smf/sml/protocol/parser.h>
#include <smf/sml/protocol/reader.h>
#include "../router.h"
#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/vm/controller.h>

namespace node
{
	class connection;
	class cache;
	class storage;
	namespace sml
	{
		class session : public std::enable_shared_from_this<session>
		{
			using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;

		public:
			session(boost::asio::ip::tcp::socket socket
				, cyng::async::mux& mux
				, cyng::logging::log_ptr logger
				, cache& cfg
				, storage& db
				, std::string const& account
				, std::string const& pwd
				, bool accept_all);

			session(session const&) = delete;
			session& operator=(session const&) = delete;

			void start();

		private:
			void do_read();

		private:
			boost::asio::ip::tcp::socket socket_;
			cyng::async::mux& mux_;
			cyng::logging::log_ptr logger_;
			cyng::controller vm_;

			/**
			 * SML parser
			 */
			sml::parser 	parser_;

			/**
			 * message dispatcher
			 */
			router router_;

			/**
			 * Buffer for incoming data.
			 */
			read_buffer_t buffer_;


		};
	}
}

#endif