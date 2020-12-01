/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_MBUS_BROKER_SESSION_H
#define NODE_MBUS_BROKER_SESSION_H

#include <NODE_project_info.h>
#include <smf/iec/parser.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller.h>

namespace node
{

	class session : public std::enable_shared_from_this<session>
	{
		using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;

	public:
		session(boost::asio::ip::tcp::socket socket
			, cyng::logging::log_ptr
			, cyng::controller&
			, boost::uuids::uuid tag);
		virtual ~session();

	public:
		void start();

	private:
		void do_read();
		void process_data(cyng::buffer_t&&);
		void process_login(cyng::buffer_t&&);

	private:
		boost::asio::ip::tcp::socket socket_;
		cyng::logging::log_ptr logger_;
		cyng::controller& cluster_;	//!< cluster bus VM
		cyng::controller vm_;	//!< session VM

		/**
		 * Buffer for incoming data.
		 */
		read_buffer_t buffer_;

		/**
		 * authorization state
		 */
		bool authorized_;

		/**
		 * temporary data buffer
		 */
		cyng::buffer_t data_;

		/**
		 * in/out byte counter
		 */
		std::uint64_t rx_, sx_;

		/**
		 * IEC parser
		 */
		iec::parser parser_;

	};
	
}

#endif
