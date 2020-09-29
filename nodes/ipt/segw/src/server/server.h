/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SEGW_SML_SERVER_H
#define NODE_SEGW_SML_SERVER_H

#include <cyng/async/mux.h>
#include <cyng/log.h>

namespace node
{
	class cache;
	class storage;
	namespace sml
	{
		class server
		{
		public:
			server(cyng::async::mux&
				, cyng::logging::log_ptr logger
				, cache& cfg
				, storage& db
				, std::string account
				, std::string pwd
				, bool accept_all
				, boost::asio::ip::tcp::endpoint ep);

			/**
			 * start listening
			 */
			void run();

			/**
			* close acceptor
			*/
			void close();

		private:
			/**
			 * Perform an asynchronous accept operation.
			 */
			void do_accept();

		private:
			/*
			 * task manager and running I/O context
			 */
			cyng::async::mux& mux_;

			/**
			 * The logger
			 */
			cyng::logging::log_ptr logger_;

			/**
			 * configuration cache
			 */
			cache& cache_;

			/**
			 * SQL database
			 */
			storage& storage_;

			/**
			 * credentials
			 */
			std::string const account_;
			std::string const pwd_;

			bool const accept_all_;

			/**
			 * Acceptor used to listen for incoming connections.
			 */
			boost::asio::ip::tcp::acceptor acceptor_;
			std::uint64_t session_counter_;

		};
	}
}

#endif

