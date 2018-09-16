/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MQTT_SERVER_H
#define NODE_MQTT_SERVER_H

#include <smf/cluster/bus.h>
#include <smf/mqtt_server_cpp.hpp>
#include <smf/mqtt/tcp_endpoint.hpp>
#include <cyng/async/mux.h>
#include <cyng/log.h>

using x = mqtt::tcp_endpoint<boost::asio::ip::tcp::socket, boost::asio::io_context::strand>;

namespace node 
{
    namespace mqtt
	{
		class server
		{
            //  namespace conflicts
//            using socket_t = ::mqtt::tcp_endpoint<boost::asio::ip::tcp::socket, boost::asio::io_context::strand>;
//            using endpoint_t = mqtt::endpoint<socket_t, std::mutex, std::lock_guard<>>;
//            using accept_handler = std::function<void(endpoint_t& ep)>;

		public:
			server(cyng::async::mux&
				, cyng::logging::log_ptr logger
                , bus::shared_type);

			/**
			 * start listening
			 */
            void run(std::string const&, std::string const&);

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
			 * cluster bus
			 */
			bus::shared_type bus_;

            /*
             * Acceptor used to listen for incoming connections.
             */
            boost::asio::ip::tcp::acceptor acceptor_;

		};
	}
}

#endif

