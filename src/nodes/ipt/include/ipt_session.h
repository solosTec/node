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
#include <cyng/vm/proxy.h>
#include <cyng/vm/vm_fwd.h>

#include <memory>
#include <array>

namespace smf {

	class ipt_session : public std::enable_shared_from_this<ipt_session>
	{
	public:
		ipt_session(boost::asio::ip::tcp::socket socket
			, bus&
			, cyng::mesh& fabric
			, ipt::scramble_key const&
			, std::uint32_t query
			, cyng::logger);
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

		void pty_res_login(bool);

		/**
		 * query some device data
		 */
		void query();

		static std::function<void(bool success)>
		get_vm_func_pty_res_login(ipt_session* p);


	private:
		boost::asio::ip::tcp::socket socket_;
		cyng::logger logger_;

		bus& cluster_bus_;
		std::uint32_t const query_;
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

		cyng::vm_proxy	vm_;

	};

}

#endif