/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_MBUS_BROKER_SESSION_H
#define NODE_MBUS_BROKER_SESSION_H

#include <NODE_project_info.h>

#include <cyng/log.h>
#include <cyng/async/mux.h>
#include <cyng/vm/controller_fwd.h>

namespace node
{

	class session : public std::enable_shared_from_this<session>
	{
		using read_buffer_t = std::array<char, NODE::PREFERRED_BUFFER_SIZE>;

	public:
		session(boost::asio::ip::tcp::socket socket
			, cyng::logging::log_ptr
			, cyng::controller&);
		virtual ~session();

	public:
		void start();

	private:
		void do_read();

	private:
		boost::asio::ip::tcp::socket socket_;
		cyng::logging::log_ptr logger_;
		cyng::controller& vm_;	//!< cluster bus VM

		/**
		 * Buffer for incoming data.
		 */
		read_buffer_t buffer_;

	};
	
}

#endif
