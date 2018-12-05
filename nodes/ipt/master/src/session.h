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

namespace node 
{
	namespace ipt
	{
		class session : public session_stub
		{
			struct connect_state
			{
				connect_state(session*, std::size_t);

				enum {
					STATE_START,		//	start or error state after failed login
					STATE_AUTHORIZED,	//	after successful login
					STATE_OFFLINE,		//	no connection at all
					STATE_LOCAL,		//	open connection to party on the same node
					STATE_REMOTE,		//	open connection to party on different node
					STATE_TASK			//	data stream linked to task
				} state_;

				/**
				 * Reference to outer session object
				 */
				session& sr_;

				/**
				 * active task
				 */
				std::size_t tsk_;

				/**
				 * Update state to authorized
				 */
				response_type set_authorized(bool b);

				/**
				 * open a local/remote connection
				 */
				void open_connection(bool);

				/**
				 * send data to a task
				 */
				void open_connection(std::size_t);

				/**
				 * Close any kind of connection
				 */
				void close_connection();

				/**
				 * Stop gatekeeper if running
				 */
				void stop();

				/**
				 * @return true if state is not START, AUTHORIZED or OFFLINE
				 */
				bool is_connected() const;

				/**
				 * @return true if state is not START.
				 */
				bool is_authorized() const;

				/**
				 * stream connection state as text
				 */
				friend std::ostream& operator<<(std::ostream& os, connect_state const& cs);

				//
				//	SML data
				//
				void register_this(cyng::controller&);
				void sml_msg(cyng::context& ctx);
				void sml_eom(cyng::context& ctx);
				void sml_public_open_response(cyng::context& ctx);
				void sml_public_close_response(cyng::context& ctx);
				void sml_get_proc_param_srv_visible(cyng::context& ctx);
				void sml_get_proc_param_srv_active(cyng::context& ctx);
				void sml_get_proc_param_firmware(cyng::context& ctx);
				void sml_get_proc_param_simple(cyng::context& ctx);
				void sml_get_proc_status_word(cyng::context& ctx);
				void sml_get_proc_param_memory(cyng::context& ctx);
				void sml_get_proc_param_wmbus_status(cyng::context& ctx);
				void sml_get_proc_param_wmbus_config(cyng::context& ctx);
				void sml_get_proc_param_ipt_status(cyng::context& ctx);
				void sml_get_proc_param_ipt_param(cyng::context& ctx);
				void sml_attention_msg(cyng::context& ctx);

			};

			friend std::ostream& operator<<(std::ostream& os, session::connect_state const& cs);

		public:
			session(boost::asio::ip::tcp::socket&& socket
				, cyng::async::mux& mux
				, cyng::logging::log_ptr logger
				, bus::shared_type bus
				, boost::uuids::uuid tag
				, std::chrono::seconds timeout
				, scramble_key const& sk
				, std::uint16_t watchdog);

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
			/**
			 * signal activity to reset watchdog
			 */
			void activity();

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
			void update_connection_state(cyng::context& ctx);
			void redirect(cyng::context& ctx);

			void client_req_gateway_proxy(cyng::context& ctx);

		private:

			/**
			 * ipt parser
			 */
			parser 	parser_;

			/**
			 * ipt serializer
			 */
			serializer		serializer_;

			const scramble_key sk_;
			const std::uint16_t watchdog_;	//!< minutes

			/**
			 * bookkeeping of ip-t sequence to task relation
			 * ipt seq => <task id / channel>
			 */
			std::map<sequence_type, std::pair<std::size_t, std::size_t>>	task_db_;

			/**
			 * contains state of local connections
			 */
			connect_state	connect_state_;

			/**
			 * proxy task
			 */
			const std::size_t proxy_tsk_;

			/**
			 * watchdog task
			 */
			std::size_t watchdog_tsk_;

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
			, std::uint16_t watchdog);

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
			using tag = std::integral_constant<std::size_t, PREDEF_CONNECTION>;
#if defined(CYNG_LEGACY_MODE_ON)
			const static char name[];
#else
			constexpr static char name[] = "ipt:session";
#endif
		};

		template <>
		struct reverse_type < PREDEF_CONNECTION >
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

