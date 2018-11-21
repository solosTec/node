/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_E350_SESSION_H
#define NODE_E350_SESSION_H

#include <smf/imega/parser.h>
#include <smf/imega/serializer.h>
#include <smf/cluster/session_stub.h>

namespace node 
{
	namespace imega
	{
		class session : public session_stub
		{
			struct connect_state
			{
				enum state {
					NOT_CONNECTED_,
					LOCAL_,
					NON_LOCAL_,

				} state_;

				connect_state();
				void set_connected(bool);
				void set_disconnected();
				bool is_local() const;
				bool is_connected() const;
			};

			std::string to_str(connect_state const&);

		public:
			session(boost::asio::ip::tcp::socket&& socket
				, cyng::async::mux& mux
				, cyng::logging::log_ptr logger
				, bus::shared_type
				, boost::uuids::uuid tag
				, std::chrono::seconds
				, bool use_global_pwd
				, std::string const& global_pwd);

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

			void imega_req_login(cyng::context& ctx);
			void imega_req_open_connection(cyng::context& ctx);
			void imega_req_transmit_data(cyng::context& ctx);
			void imega_res_watchdog(cyng::context& ctx);

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
			void update_connection_state(cyng::context& ctx);


		private:
			/**
			 * iMEGA parser
			 */
			parser 	parser_;

			/**
			 * wrapper class to serialize and send
			 * data and code.
			 */
			serializer		serializer_;

			/**
			 * gatekeeper task
			 */
			std::size_t gate_keeper_;

			const bool use_global_pwd_;
			const std::string global_pwd_;

			/**
			 * contains state of local connections
			 */
			connect_state	connect_state_;

		};

		cyng::object make_session(boost::asio::ip::tcp::socket&& socket
			, cyng::async::mux& mux
			, cyng::logging::log_ptr logger
			, bus::shared_type
			, boost::uuids::uuid tag
			, std::chrono::seconds
			, bool use_global_pwd
			, std::string const& global_pwd);
	}
}

#include <cyng/intrinsics/traits.hpp>
namespace cyng
{
	namespace traits
	{
		template <>
		struct type_tag<node::imega::session>
		{
			using type = node::imega::session;
			using tag = std::integral_constant<std::size_t, PREDEF_CONNECTION>;
#if defined(CYNG_LEGACY_MODE_ON)
			const static char name[];
#else
			constexpr static char name[] = "ipt:connection";
#endif
		};

		template <>
		struct reverse_type < PREDEF_CONNECTION >
		{
			using type = node::imega::session;
		};
	}
}

#include <functional>
#include <boost/functional/hash.hpp>

namespace std
{
	template<>
	struct hash<node::imega::session>
	{
		inline size_t operator()(node::imega::session const& conn) const noexcept
		{
			return conn.hash();
		}
	};
	template<>
	struct equal_to<node::imega::session>
	{
		using result_type = bool;
		using first_argument_type = node::imega::session;
		using second_argument_type = node::imega::session;

		inline bool operator()(first_argument_type const& conn1, second_argument_type const& conn2) const noexcept
		{
			return conn1.hash() == conn2.hash();
		}
	};
	template<>
	struct less<node::imega::session>
	{
		using result_type = bool;
		using first_argument_type = node::imega::session;
		using second_argument_type = node::imega::session;

		inline bool operator()(first_argument_type const& conn1, second_argument_type const& conn2) const noexcept
		{
			return conn1.hash() < conn2.hash();
		}
	};
}

#endif

