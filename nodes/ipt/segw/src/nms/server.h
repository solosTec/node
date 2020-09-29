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

#include <boost/uuid/uuid_generators.hpp>

namespace node
{
	namespace nms
	{
		class server
		{
		public:
			server(cyng::io_service_t&
				, cyng::logging::log_ptr
				//, cyng::controller&
				, boost::asio::ip::tcp::endpoint ep);

		public:
			void run();
			void close();

		private:
			void do_accept();

		private:
			boost::asio::ip::tcp::acceptor acceptor_;
			cyng::logging::log_ptr logger_;
			//cyng::controller& vm_;
			std::uint64_t session_counter_;
			boost::uuids::random_generator uuid_gen_;
		};
	}
}

#endif
