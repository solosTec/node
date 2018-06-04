/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MASTER_SERVER_H
#define NODE_MASTER_SERVER_H

#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/store/db.h>
#include <unordered_map>
#include <atomic>
#include <boost/version.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>

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
			, int monitor
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

		//	master tag
		const boost::uuids::uuid tag_;	
		//	credentials
		const std::string account_;
		const std::string pwd_;
		const std::chrono::seconds monitor_;	//!< cluster monitor

		//	connection parameters
		std::atomic<std::chrono::seconds> connection_open_timeout_;
		std::atomic<std::chrono::seconds> connection_close_timeout_;
		std::atomic<bool> connection_auto_login_;
		std::atomic<bool> connection_auto_enabled_;
		std::atomic<bool> connection_superseed_;

		/// Acceptor used to listen for incoming connections.
		boost::asio::ip::tcp::acceptor acceptor_;		

#if (BOOST_VERSION < 106600)
		boost::asio::ip::tcp::socket socket_;
#endif

		/**
		 * database
		 */
		cyng::store::db	db_;

		/**
		 * generate session tags
		 */
		boost::uuids::random_generator rgn_;
	};
}

#endif

