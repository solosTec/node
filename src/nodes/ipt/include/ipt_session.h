/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_SESSION_H
#define SMF_IPT_SESSION_H

#include <smf/ipt/parser.h>
#include <smf/cluster/bus.h>
#include <smf/ipt/serializer.h>

#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/buffer.h>

#include <memory>
#include <array>

namespace smf {

	class ipt_server;
	class ipt_session : public std::enable_shared_from_this<ipt_session>
	{
	public:
		ipt_session(boost::asio::ip::tcp::socket socket, ipt_server*, cyng::logger);
		~ipt_session();

		void start();
		void stop();

	private:
		void do_read();
		void do_write();
		void handle_write(const boost::system::error_code& ec);

		//
		//	bus interface
		//
		void ipt_cmd(ipt::header const&, cyng::buffer_t&&);
		void ipt_stream(cyng::buffer_t&&);

	private:
		boost::asio::ip::tcp::socket socket_;
		cyng::logger logger_;

		bus& cluster_bus_;

		/**
		 * Buffer for incoming data.
		 */
		std::array<char, 2048>	buffer_;

		/**
		 * Buffer for outgoing data.
		 */
		std::deque<cyng::buffer_t>	buffer_write_;

		/**
		 * parser for ip-t data
		 */
		ipt::parser parser_;

		ipt::serializer	serializer_;

	};

}

#endif
