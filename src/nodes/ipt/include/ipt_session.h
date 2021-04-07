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
		/**
		 * start an async write
		 */
		void ipt_send(cyng::buffer_t&&);

		void pty_res_login(bool, boost::uuids::uuid dev);
		void pty_res_register(bool, std::uint32_t, cyng::param_map_t token);

		/**
		 * query some device data
		 */
		void query();

		void update_software_version(std::string);
		void update_device_identifier(std::string);
		void register_target(std::string name
			, std::uint16_t paket_size
			, std::uint8_t window_size
			, boost::uuids::uuid tag
			, ipt::sequence_t);

		void deregister_target(std::string name
			, boost::uuids::uuid tag
			, ipt::sequence_t);

		void open_push_channel(std::string target
			, std::string account
			, std::string msisdn
			, std::string version
			, std::string id
			, std::uint16_t timeout
			, boost::uuids::uuid tag
			, ipt::sequence_t);

		void close_push_channel(std::uint32_t channel
			, boost::uuids::uuid tag
			, ipt::sequence_t);

		static std::function<void(bool success, boost::uuids::uuid)>
		get_vm_func_pty_res_login(ipt_session* p);

		static std::function<void(bool success, std::uint32_t, cyng::param_map_t)>
		get_vm_func_pty_res_register(ipt_session* p);

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

		/**
		 * tag/pk of device
		 */
		boost::uuids::uuid dev_;
	};

}

#endif
