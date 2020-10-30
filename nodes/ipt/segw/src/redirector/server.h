/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_MBUS_RS485_SERVER_H
#define NODE_MBUS_RS485_SERVER_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller_fwd.h>

namespace node
{
	class cache;
	namespace redirector
	{
		class server
		{
		public:
			server(cyng::async::mux&
				, cyng::logging::log_ptr
				, cache& cfg
				, std::string account
				, std::string pwd
				, boost::asio::ip::tcp::endpoint ep);

		public:
			void run();
			void close();

		private:
			void do_accept();

		private:
			cyng::async::mux& mux_;
			cyng::logging::log_ptr logger_;

			/**
			 * configuration cache
			 */
			cache& cache_;

			/**
			 * credentials
			 */
			std::string const account_;
			std::string const pwd_;

			boost::asio::ip::tcp::acceptor acceptor_;
			std::uint64_t session_counter_;
		};
	}
}

#endif
