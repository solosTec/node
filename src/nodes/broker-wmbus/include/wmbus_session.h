/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_TASK_WMBUS_SESSION_H
#define SMF_TASK_WMBUS_SESSION_H

#include <cyng/log/logger.h>
#include <cyng/io/parser/parser.h>
#include <cyng/vm/proxy.h>
#include <cyng/obj/intrinsics/pid.h>

#include <memory>
#include <array>

namespace smf {

	class wmbus_server;
	class wmbus_session : public std::enable_shared_from_this<wmbus_session>
	{
	public:
		wmbus_session(boost::asio::ip::tcp::socket socket, wmbus_server*, cyng::logger);
		~wmbus_session();

		void start();
		void stop();

	private:
		void do_read();
		void do_write();
		void handle_write(const boost::system::error_code& ec);

	private:
		boost::asio::ip::tcp::socket socket_;
		cyng::logger logger_;

		/**
		 * Buffer for incoming data.
		 */
		std::array<char, 2048>	buffer_;

		/**
		 * Buffer for outgoing data.
		 */
		std::deque<cyng::buffer_t>	buffer_write_;

	};

}

#endif
