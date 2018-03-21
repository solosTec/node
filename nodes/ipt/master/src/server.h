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
#include <boost/functional/hash.hpp>
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

			void insert_connection(cyng::context&);
			void close_connection(cyng::context&);

			void client_res_login(cyng::context&);
			void client_res_close_impl(cyng::context&);
			void client_req_close_impl(cyng::context&);
			void client_res_open_push_channel(cyng::context&);
			void client_res_register_push_target(cyng::context&);
			void client_res_open_connection(cyng::context&);
			void client_req_open_connection_forward(cyng::context&);
			void client_res_open_connection_forward(cyng::context&);
			void client_req_transmit_data_forward(cyng::context&);
			void client_res_transfer_pushdata(cyng::context& ctx);
			void client_req_transfer_pushdata_forward(cyng::context& ctx);
			void client_res_close_push_channel(cyng::context& ctx);
			void client_req_close_connection_forward(cyng::context& ctx);

			void propagate(std::string, cyng::vector_t&&);

			/**
			 * create a reference of this object on stack.
			 */
			void push_connection(cyng::context& ctx);
			void push_ep_local(cyng::context& ctx);
			void push_ep_remote(cyng::context& ctx);

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
			std::map<boost::uuids::uuid, cyng::object>	client_map_;

		};
	}
}

#endif

