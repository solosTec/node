/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_MBUS_BROKER_SERVER_H
#define NODE_MBUS_BROKER_SERVER_H

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller_fwd.h>

namespace node
{
	class cache;
	namespace nms
	{
		class server
		{
		public:
			server(cyng::io_service_t&
				, cyng::logging::log_ptr
				, cache& cfg
				, std::string account
				, std::string pwd
				, bool accept_all
				, boost::asio::ip::tcp::endpoint ep);

		public:
			void run();
			void close();

		private:
			void do_accept();

		private:
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

			bool const accept_all_;

			boost::asio::ip::tcp::acceptor acceptor_;
			std::uint64_t session_counter_;
		};
	}
}

#endif
