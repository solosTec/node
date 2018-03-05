/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_GATEWAY_SERVER_H
#define NODE_IPT_GATEWAY_SERVER_H

#include <cyng/async/mux.h>
#include <cyng/log.h>
//#include <cyng/store/db.h>
#include <boost/uuid/uuid.hpp>
#include <unordered_map>

namespace node 
{
	class server 
	{
	public:
		server(cyng::async::mux&
			, cyng::logging::log_ptr logger
			, boost::uuids::uuid
			, std::string account
			, std::string pwd
			, cyng::tuple_t cfg_session);

		/**
		 * start listening
		 */
		void run(std::string const&, std::string const&);
		
		/**
		 * close acceptor 
		 */
		void close();
		
	private:
		/// Perform an asynchronous accept operation.
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

		//	credentials
		const std::string account_;
		const std::string pwd_;
		const std::chrono::seconds monitor_;

		
		/// Acceptor used to listen for incoming connections.
		boost::asio::ip::tcp::acceptor acceptor_;		

#if (BOOST_VERSION < 106600)
		boost::asio::ip::tcp::socket socket_;
#endif


	};
}

#endif

