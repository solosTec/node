/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MAIN_SESSION_H
#define SMF_MAIN_SESSION_H

#include <cyng/log/logger.h>
#include <cyng/io/parser/parser.h>
#include <cyng/vm/proxy.h>
//#include <cyng/vm/mesh.h>
#include <cyng/obj/intrinsics/pid.h>

#include <memory>
#include <array>

namespace smf {

	class server;
	class session : public std::enable_shared_from_this<session>
	{
	public:
		session(boost::asio::ip::tcp::socket socket, server*, cyng::logger);
		~session();

		void start();
		void stop();

	private:
		cyng::vm_proxy init_vm(server*);
		void do_read();
		void do_write();
		void handle_write(const boost::system::error_code& ec);

		void cluster_login(std::string, std::string, cyng::pid, std::string node, boost::uuids::uuid tag);
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

		cyng::vm_proxy	vm_;
		cyng::io::parser parser_;

	};

}

#endif
