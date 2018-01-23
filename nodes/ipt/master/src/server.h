/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_MASTER_SERVER_H
#define NODE_IPT_MASTER_SERVER_H

#include <smf/cluster/bus.h>
#include <smf/ipt/scramble_key.h>
#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <cyng/store/db.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <unordered_map>

namespace node 
{
	namespace ipt
	{
		class connection;
		class server
		{
		public:
			server(cyng::async::mux&
				, cyng::logging::log_ptr logger
				, bus::shared_type
				, scramble_key const& sk
				, uint16_t watchdog
				, int timeout);

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
			void client_res_login(cyng::context&);
			void client_res_open_push_channel(cyng::context&);

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
			 * cluster bus
			 */
			bus::shared_type bus_;

			 //	configuration
			 const scramble_key sk_;
			 const std::uint16_t watchdog_;
			 const std::chrono::seconds timeout_;


			/// Acceptor used to listen for incoming connections.
			boost::asio::ip::tcp::acceptor acceptor_;

#if (BOOST_VERSION < 106600)
			boost::asio::ip::tcp::socket socket_;
#endif
			boost::uuids::random_generator rnd_;

			/**
			 * client map
			 */
			std::map<boost::uuids::uuid, std::shared_ptr<connection> >	client_map_;

		};
	}
}

#endif

