/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_SERVER_STUB_H
#define NODE_SERVER_STUB_H

#include <smf/cluster/bus.h>
#include <cyng/async/mux.h>
#include <cyng/log.h>
#include <boost/uuid/random_generator.hpp>

namespace node 
{
	/**
	 * This is a stub implementation for all servers that are communicate
	 * with the SMF cluster and manage a bunch of clients.
	 */
	class server_stub
	{
	public:
		server_stub(cyng::async::mux&
			, cyng::logging::log_ptr logger
			, bus::shared_type
			, std::chrono::seconds);

		/**
		 * start listening
		 */
		void run(std::string const&, std::string const&);

		/**
		 * close acceptor
		 */
		void close();

	protected:
		virtual cyng::object make_client(boost::uuids::uuid tag, boost::asio::ip::tcp::socket socket) = 0;
		virtual bool close_connection(boost::uuids::uuid tag, cyng::object) = 0;
		virtual void start_client(cyng::object) = 0;
		virtual void propagate(cyng::object, cyng::vector_t&& msg) = 0;

		/**
		 * Generic method to propagate a function call.
		 * Assumes that the first value of the program vector contains
		 * a client tag.
		 */
		void client_propagate(cyng::context&);

	private:
		/**
		 * Perform an asynchronous accept operation.
		 */
		void do_accept();
		void create_client(boost::asio::ip::tcp::socket socket);

		/**
		 * @return true if entry was found
		 */
		bool clear_connection_map_impl(boost::uuids::uuid);
		void clear_connection_map(cyng::context& ctx);

		void shutdown_clients(cyng::context& ctx);

		void transmit_data(cyng::context& ctx);	//!< transmit data locally


		void client_res_close_impl(cyng::context&);
		void client_req_close_impl(cyng::context&);
		void client_res_open_connection_forward(cyng::context&);

		void propagate(std::string, cyng::vector_t const&);

		/**
		 * Search for client tag in client_map_ and propagate function call
		 * to this client.
		 *
		 * @param fn function name
		 * @param tag client tag
		 * @param prg program vector
		 */
		void propagate(std::string fn, boost::uuids::uuid tag, cyng::vector_t&& prg);

		/**
		 * create a reference of this object on stack.
		 */
		void push_connection(cyng::context& ctx);

		/**
		 * Remove a connection/client from connection list.
		 */
		void remove_client(cyng::context&);
		bool remove_client_impl(boost::uuids::uuid tag);

		void close_client(cyng::context&);
		bool close_client_impl(boost::uuids::uuid tag);

		/**
		 * insert IP-T sessions
		 */
		void insert_client(cyng::context&);
		bool insert_client_impl(boost::uuids::uuid tag, cyng::object obj);

		void close_acceptor();
		void close_clients();

	protected:
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

		/**
		 * Timeout for cluster bus operations.
		 * A timeout is required to detect missing responses.
		 */
		const std::chrono::seconds timeout_;

	private:
		/** 
		 * Acceptor used to listen for incoming connections.
		 */
		boost::asio::ip::tcp::acceptor acceptor_;

#if (BOOST_VERSION < 106600)
		boost::asio::ip::tcp::socket socket_;
#endif
		/**
		 * Generate client tags
		 */
		boost::uuids::random_generator rnd_;

		/**
		 * client map:
		 * Establishes the connection between an UUID and a connection/session object.
		 */
		std::map<boost::uuids::uuid, cyng::object>	client_map_;

		/**
		 * connection map:
		 * Contains all local connections.
		 */
		std::map<boost::uuids::uuid, boost::uuids::uuid> connection_map_;

		/**
		 *	Synchronisation objects for proper shutdown
		 */
		cyng::async::condition_variable cv_acceptor_closed_, cv_sessions_closed_;
		cyng::async::mutex mutex_;

	};
}

#endif

