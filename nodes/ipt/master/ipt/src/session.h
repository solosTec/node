/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_MASTER_SESSION_H
#define NODE_IPT_MASTER_SESSION_H

#include <smf/ipt/parser.h>
#include <smf/ipt/serializer.h>
#include <smf/cluster/session_stub.h>
#include "session_state.h"
#include "proxy_comm.h"

namespace node 
{
	namespace ipt
	{
		/**
		 * compile with SMF_IO_LOG to generate full I/O logs
		 */
		class session : public session_stub
		{
			//
			//	allow state class to manager the inner state of the session
			//
			friend class session_state;

		public:
			session(boost::asio::ip::tcp::socket&& socket
				, cyng::async::mux& mux
				, cyng::logging::log_ptr logger
				, bus::shared_type bus
				, boost::uuids::uuid tag
				, std::chrono::seconds timeout
				, scramble_key const& sk
				, std::uint16_t watchdog
				, bool sml_log);

			session(session const&) = delete;
			session& operator=(session const&) = delete;

			virtual ~session();

		protected:
			/**
			 * stop running tasks and halt VM
			 */
			virtual void shutdown();

			virtual cyng::buffer_t parse(read_buffer_const_iterator, read_buffer_const_iterator) override;

		private:
			void ipt_req_login_public(cyng::context& ctx);
			void ipt_req_login_scrambled(cyng::context& ctx);
			void ipt_req_logout(cyng::context& ctx);
			void ipt_res_logout(cyng::context& ctx);

			void ipt_req_open_push_channel(cyng::context& ctx);

			/**
			 * Normally ipt_res_open_push_channel() should not be used, because the open
			 * push channel request is answered by the IP-T master.
			 * There is a bug in the VARIOSafe Manager to answer an open
			 * connection request with an open push channel response.
			 */
			void ipt_res_open_push_channel(cyng::context& ctx);
			void ipt_req_close_push_channel(cyng::context& ctx);
			void ipt_req_open_connection(cyng::context& ctx);
			void ipt_req_close_connection(cyng::context& ctx);
			void ipt_res_open_connection(cyng::context& ctx);
			void ipt_req_transfer_pushdata(cyng::context& ctx);	//!< 0x9002
			void ipt_res_transfer_pushdata(cyng::context& ctx);	//!< 0x1002
			void ipt_res_close_connection(cyng::context& ctx);
			void ipt_req_transmit_data(cyng::context& ctx);
			void ipt_req_watchdog(cyng::context& ctx);
			void ipt_res_watchdog(cyng::context& ctx);

			void ipt_req_protocol_version(cyng::context& ctx);
			void ipt_res_protocol_version(cyng::context& ctx);
			void ipt_req_software_version(cyng::context& ctx);
			void ipt_res_software_version(cyng::context& ctx);
			void ipt_res_dev_id(cyng::context& ctx);
			void ipt_req_device_id(cyng::context& ctx);
			void ipt_req_net_stat(cyng::context& ctx);
			void ipt_res_network_stat(cyng::context& ctx);
			void ipt_req_ip_statistics(cyng::context& ctx);
			void ipt_res_ip_statistics(cyng::context& ctx);
			void ipt_req_dev_auth(cyng::context& ctx);
			void ipt_res_dev_auth(cyng::context& ctx);
			void ipt_req_dev_time(cyng::context& ctx);
			void ipt_res_dev_time(cyng::context& ctx);

			void ipt_unknown_cmd(cyng::context& ctx);

			void client_res_login(cyng::context& ctx);
			void client_res_open_push_channel(cyng::context& ctx);
			void client_res_close_push_channel(cyng::context& ctx);
			void client_req_open_connection_forward(cyng::context& ctx);
			void client_res_open_connection_forward(cyng::context& ctx);
			void client_req_transmit_data_forward(cyng::context& ctx);
			void client_res_transfer_pushdata(cyng::context& ctx);
			void client_req_transfer_pushdata_forward(cyng::context& ctx);
			void client_req_close_connection_forward(cyng::context& ctx);
			void client_res_close_connection_forward(cyng::context& ctx);

			void ipt_req_register_push_target(cyng::context& ctx);
			void client_res_register_push_target(cyng::context& ctx);
			void ipt_req_deregister_push_target(cyng::context& ctx);
			void client_res_deregister_push_target(cyng::context& ctx);

			void client_res_open_connection(cyng::context& ctx);

			void store_relation(cyng::context& ctx);
			void remove_relation(cyng::context& ctx);

			void client_req_gateway(cyng::context& ctx);

		private:

			/**
			 * ipt parser
			 */
			parser 	parser_;

			/**
			 * ipt serializer
			 */
			serializer	serializer_;

			/**
			 * response timeout
			 */
			const std::chrono::seconds timeout_;

			/**
			 * bookkeeping of ip-t sequence to task relation
			 * ipt seq => <task id / channel>
			 */
			std::map<sequence_type, std::pair<std::size_t, std::size_t>>	task_db_;

			/**
			 * contains state of local connections
			 */
			session_state	state_;

			/**
			 * SML communication
			 */
			proxy_comm	proxy_comm_;

#ifdef SMF_IO_LOG
			std::size_t log_counter_;
#endif
		};

		cyng::object make_session(boost::asio::ip::tcp::socket&& socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type bus
			, boost::uuids::uuid tag
			, std::chrono::seconds const& timeout
			, scramble_key const& sk
			, std::uint16_t watchdog
			, bool sml_log);

	}
}

#include <cyng/intrinsics/traits.hpp>
namespace cyng
{
	namespace traits
	{
		template <>
		struct type_tag<node::ipt::session>
		{
			using type = node::ipt::session;
			using tag = std::integral_constant<std::size_t,
#if defined(__CPP_SUPPORT_N2347)
				static_cast<std::size_t>(traits::predef_type_code::PREDEF_CONNECTION)
#else
				PREDEF_CONNECTION
#endif
			>;

#if defined(__CPP_SUPPORT_N2235)
			constexpr static char name[] = "ipt:session";
#else
			const static char name[];
#endif
		};

		template <>
		struct reverse_type <
#if defined(__CPP_SUPPORT_N2347)
			static_cast<std::size_t>(traits::predef_type_code::PREDEF_CONNECTION)
#else
			PREDEF_CONNECTION
#endif
		>
		{
			using type = node::ipt::session;
		};
	}
}

#include <functional>
#include <boost/functional/hash.hpp>

namespace std
{
	template<>
	struct hash<node::ipt::session>
	{
		inline size_t operator()(node::ipt::session const& conn) const noexcept
		{
			return conn.hash();
		}
	};
	template<>
	struct equal_to<node::ipt::session>
	{
		using result_type = bool;
		using first_argument_type = node::ipt::session;
		using second_argument_type = node::ipt::session;

		inline bool operator()(first_argument_type const& conn1, second_argument_type const& conn2) const noexcept
		{
			return conn1.hash() == conn2.hash();
		}
	};
	template<>
	struct less<node::ipt::session>
	{
		using result_type = bool;
		using first_argument_type = node::ipt::session;
		using second_argument_type = node::ipt::session;

		inline bool operator()(first_argument_type const& conn1, second_argument_type const& conn2) const noexcept
		{
			return conn1.hash() < conn2.hash();
		}
	};
}

#endif

