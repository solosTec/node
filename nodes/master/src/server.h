/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MASTER_SERVER_H
#define NODE_MASTER_SERVER_H

#include <cyng/compatibility/io_service.h>
#include <cyng/log.h>

namespace node 
{
	class server 
	{
	public:
		server(cyng::io_service_t&, cyng::logging::log_ptr logger);
		void run(std::string const&, std::string const&);
		
		/**
		 * close acceptor 
		 */
		void close();
		
	private:
		/// Perform an asynchronous accept operation.
		void do_accept();
		
	private:
		/// The io_context used to perform asynchronous operations.
		cyng::io_service_t& io_ctx_;
		
		/// The logger
		cyng::logging::log_ptr logger_;
		
		/// Acceptor used to listen for incoming connections.
		boost::asio::ip::tcp::acceptor acceptor_;		

#if (BOOST_VERSION < 106600)
		boost::asio::ip::tcp::socket socket_;
#endif

	};
}

#endif

