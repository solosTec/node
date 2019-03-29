/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_MODEM_SESSION_H
#define NODE_MODEM_SESSION_H

#include <smf/modem/parser.h>
#include <smf/modem/serializer.h>
#include <smf/cluster/session_stub.h>
#include "session_state.h"

namespace node 
{
	namespace modem
	{
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
				, bus::shared_type
				, boost::uuids::uuid tag
				, std::chrono::seconds const& timeout
				, bool auto_answer
				, std::chrono::milliseconds guard_time);

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

			void modem_req_login(cyng::context& ctx);
			//void ipt_req_logout(cyng::context& ctx);
			//void ipt_res_logout(cyng::context& ctx);

			//void ipt_req_open_push_channel(cyng::context& ctx);
			//void ipt_req_close_push_channel(cyng::context& ctx);
			void modem_req_open_connection(cyng::context& ctx);
			void modem_req_close_connection(cyng::context& ctx);
			//void ipt_res_open_connection(cyng::context& ctx);
			//void ipt_req_transfer_pushdata(cyng::context& ctx);
			//void ipt_res_close_connection(cyng::context& ctx);
			void modem_req_transmit_data(cyng::context& ctx);
			//void ipt_res_watchdog(cyng::context& ctx);

			//void ipt_res_protocol_version(cyng::context& ctx);
			//void ipt_res_software_version(cyng::context& ctx);
			//void ipt_res_dev_id(cyng::context& ctx);
			//void ipt_res_network_stat(cyng::context& ctx);
			//void ipt_res_ip_statistics(cyng::context& ctx);
			//void ipt_res_dev_auth(cyng::context& ctx);
			//void ipt_res_dev_time(cyng::context& ctx);

			//void ipt_unknown_cmd(cyng::context& ctx);

			//void client_res_login(std::uint64_t, bool, std::string msg);
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

			//void ipt_req_register_push_target(cyng::context& ctx);
			void client_res_register_push_target(cyng::context& ctx);

			void client_res_open_connection(cyng::context& ctx);

			void store_relation(cyng::context& ctx);
			//void update_connection_state(cyng::context& ctx);

			void modem_req_info(cyng::context& ctx);

		private:
			/**
			 * modem parser
			 */
			parser 	parser_;

			/**
			 * modem serializer
			 */
			serializer 	serializer_;

			/**
			 * contains state of local connections
			 */
			session_state	state_;
		};

		cyng::object make_session(boost::asio::ip::tcp::socket&& socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type
			, boost::uuids::uuid tag
			, std::chrono::seconds const& timeout
			, bool auto_answer
			, std::chrono::milliseconds guard_time);

	}
}

#include <cyng/intrinsics/traits.hpp>
namespace cyng
{
	namespace traits
	{
		template <>
		struct type_tag<node::modem::session>
		{
			using type = node::modem::session;
			using tag = std::integral_constant<std::size_t, 
#if defined(__CPP_SUPPORT_N2347)
				static_cast<std::size_t>(traits::predef_type_code::PREDEF_CONNECTION)
#else
				PREDEF_CONNECTION
#endif
			>;

#if defined(__CPP_SUPPORT_N2235)
			constexpr static char name[] = "ipt:connection";
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
			using type = node::modem::session;
		};
	}
}

#include <functional>
#include <boost/functional/hash.hpp>

namespace std
{
	template<>
	struct hash<node::modem::session>
	{
		inline size_t operator()(node::modem::session const& conn) const noexcept
		{
			return conn.hash();
		}
	};
	template<>
	struct equal_to<node::modem::session>
	{
		using result_type = bool;
		using first_argument_type = node::modem::session;
		using second_argument_type = node::modem::session;

		inline bool operator()(first_argument_type const& conn1, second_argument_type const& conn2) const noexcept
		{
			return conn1.hash() == conn2.hash();
		}
	};
	template<>
	struct less<node::modem::session>
	{
		using result_type = bool;
		using first_argument_type = node::modem::session;
		using second_argument_type = node::modem::session;

		inline bool operator()(first_argument_type const& conn1, second_argument_type const& conn2) const noexcept
		{
			return conn1.hash() < conn2.hash();
		}
	};
}

#endif

