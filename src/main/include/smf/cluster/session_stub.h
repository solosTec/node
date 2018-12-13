/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_SESSION_STUB_H
#define NODE_SESSION_STUB_H

#include <smf/cluster/bus.h>
#include <smf/cluster/generator.h>

#include <cyng/log.h>
#include <cyng/vm/generator.h>
#include <cyng/object_cast.hpp>

namespace node 
{
	/**
	 * base class for sessions on cluster clients.
	 */
	class session_stub
	{
	protected:
		using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;
		using read_buffer_iterator = read_buffer_t::iterator;
		using read_buffer_const_iterator = read_buffer_t::const_iterator;

		/**
		 * connection socket
		 */
		boost::asio::ip::tcp::socket socket_;

	private:

		/**
		 * Buffer for incoming data.
		 */
		read_buffer_t buffer_;

		/**
		 * session is in shutdown mode
		 */
		bool pending_;

	protected:

		/**
		 * scheduler 
		 */
		cyng::async::mux& mux_;

		/**
		 * The logger instance
		 */
		cyng::logging::log_ptr logger_;

		/**
		 * cluster bus
		 */
		bus::shared_type bus_;

		/**
		 * device specific functions and state
		 */
		cyng::controller vm_;	

		/**
		 * operational timeout
		 */
		const std::chrono::seconds timeout_;

	public:
		session_stub(boost::asio::ip::tcp::socket&& socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, boost::uuids::uuid tag
			, std::chrono::seconds timeout);

		session_stub(session_stub const&) = delete;
		session_stub& operator=(session_stub const&) = delete;

		~session_stub();

		/**
		 * start reading from socket
		 */
		void start();

		/**
		 * close socket.
		 * Same as "ip.tcp.socket.shutdown" and "ip.tcp.socket.close"
		 */
		void close();

		cyng::controller& vm();
		cyng::controller const& vm() const;

		/**
		 * @return connection specific hash based in internal tag
		 */
		std::size_t hash() const noexcept;

	protected:
		virtual cyng::buffer_t parse(read_buffer_const_iterator, read_buffer_const_iterator) = 0;

		/**
		 * halt VM
		 */
		virtual void shutdown();

	private:
		void stop(boost::system::error_code ec);
		void do_read();
	};
}


#endif
