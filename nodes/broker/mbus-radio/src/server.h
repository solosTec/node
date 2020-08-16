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

namespace node
{

	class server
	{
	public:
		server(cyng::io_service_t&
			, cyng::logging::log_ptr
			, boost::asio::ip::tcp::endpoint ep);

	public:
		void run();
		void close();

	private:
		void do_accept();

	private:
		boost::asio::ip::tcp::acceptor acceptor_;
		//cyng::io_service_t& ios_;
		cyng::logging::log_ptr logger_;
		//boost::asio::ip::tcp::endpoint const ep_;
	};
	
}

#endif
