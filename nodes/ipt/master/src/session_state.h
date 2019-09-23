/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 

#ifndef NODE_IPT_MASTER_SESSION_STATE_H
#define NODE_IPT_MASTER_SESSION_STATE_H

#include <smf/cluster/bus.h>
#include <smf/ipt/scramble_key.h>
#include <smf/ipt/defs.h>
#include <cyng/async/mux.h>
#include <cyng/log.h>


/** @brief session states
 * @verbatim

   +---------------------------+                                          +---------------------------+
   |                           |                                          |                           |
   | offline                   |                                          | offline                   |
   |                           |                                          |                           |
   +---------------------------+                                          +---------------------------+

   +---------------------------+                                          +---------------------------+
   |                           |                                          |                           |
   | authorized                |                                          | authorized                |
   |                           |                                          |                           |
   |                           |                                          +---------------------------+
   |                           |
   |                           |                                          +---------------------------+
   |                           |   "client.req.open.connection"           |                           | "req.open.connection"
   |                           +----------------------------------------> | wait for open response    +----------------------------+
   |                           |   "client.req.open.connection.forward"   |                           |                            |
   +---------------------------+                                          +---------------------------+                            |
																																   |
   +---------------------------+                                          +---------------------------+                            |
   |                           |   "client.res.open.connection"           |                           |                            |
   | connected                 | <----------------------------------------+ connected                 | <--------------------------+
   |                           |   "client.res.open.connection.forward"   |                           |  "ipt.res.open.connection"
   |                           |                                          +---------------------------+
   |                           |
   |                           |                                          +---------------------------+
   |                           |   "client.req.close.connection"          |                           |  "res.close.connection"
   |                           +----------------------------------------> | wait for close response   +----------------------------+
   |                           |   "client.req.close.connection.forward"  |                           |                            |
   +---------------------------+                                          +---------------------------+                            |
																																   |
   +---------------------------+                                          +---------------------------+                            |
   |                           |   "client.res.close.connection"          |                           |                            |
   | authorized                | <----------------------------------------+ authorized                | <--------------------------+
   |                           |   "client.res.close.connection.forward"  |                           |  "ipt.res.close.connection"
   +---------------------------+                                          +---------------------------+

 * @endverbatim
 */


namespace node 
{
	namespace ipt
	{
		//
		//	forwward declarations
		//
		class session;

		namespace state
		{
			struct evt_init_complete
			{
				const std::size_t tsk_;
				const bool success_;
				const std::uint16_t watchdog_;
				evt_init_complete(std::pair<std::size_t, bool>, std::uint16_t);
			};

			/**
			 * device sends public login request
			 */
			struct evt_req_login_public
			{
				boost::uuids::uuid const tag_;
				std::string const name_, pwd_;
				boost::asio::ip::tcp::endpoint const lep_;
				boost::asio::ip::tcp::endpoint const rep_;

				evt_req_login_public(std::tuple
					<
						boost::uuids::uuid,		//	[0] peer tag
						std::string,			//	[1] name
						std::string,			//	[2] pwd
						boost::asio::ip::tcp::endpoint,	//	[3] local ep
						boost::asio::ip::tcp::endpoint	//	[4] remote ep
					>);
			};

			/**
			 * device sends scrambled login request
			 */
			struct evt_req_login_scrambled
			{
				boost::uuids::uuid const tag_;
				std::string const name_, pwd_;
				scramble_key const sk_;
				boost::asio::ip::tcp::endpoint const lep_;
				boost::asio::ip::tcp::endpoint const rep_;
				evt_req_login_scrambled(boost::uuids::uuid
					, std::string const&
					, std::string const&
					, scramble_key const&
					, boost::asio::ip::tcp::endpoint
					, boost::asio::ip::tcp::endpoint);
			};

			/**
			 * master sends login response
			 */
			struct evt_res_login
			{
				const boost::uuids::uuid tag_;	//	[0] peer tag
				const std::uint64_t seq_;		//	[1] sequence number
				const bool success_;			//	[2] success
				const std::string name_;		//	[3] name
				const std::string msg_;			//	[4] msg
				const std::uint32_t query_;		//	[5] query
				const cyng::param_map_t bag_;	//	[6] bag

				evt_res_login(std::tuple<
					boost::uuids::uuid,		//	[0] peer tag
					std::uint64_t,			//	[1] sequence number
					bool,					//	[2] success
					std::string,			//	[3] name
					std::string,			//	[4] msg
					std::uint32_t,			//	[5] query
					cyng::param_map_t		//	[6] bag
				>);

			};

			/**
			 * device sends connection open request
			 */
			struct evt_ipt_req_open_connection
			{
				const boost::uuids::uuid tag_;
				const sequence_type seq_;
				const std::string number_;

				evt_ipt_req_open_connection(boost::uuids::uuid,
					sequence_type,
					std::string);
			};

			/**
			 * device sends connection open response
			 */
			struct evt_ipt_res_open_connection
			{
				std::size_t const tsk_;
				std::size_t const slot_;
				response_type const res_;
				evt_ipt_res_open_connection(std::size_t, std::size_t, response_type);
			};

			/**
			 * device sends connection close request
			 */
			struct evt_ipt_req_close_connection 
			{
				const boost::uuids::uuid tag_;		//	[0] session tag
				const sequence_type seq_;			//	[1] seq
				evt_ipt_req_close_connection(std::tuple<
					boost::uuids::uuid,		//	[0] session tag
					sequence_type			//	[1] seq
				>);
			};

			/**
			 * device sends connection close response
			 */
			struct evt_ipt_res_close_connection 
			{
				std::size_t const tsk_;
				std::size_t const slot_;
				response_type const res_;
				evt_ipt_res_close_connection(std::size_t, std::size_t, response_type);
			};

			/**
			 * incoming connection open request
			 */
			struct evt_client_req_open_connection
			{
				const std::size_t tsk_;
				const bool success_;
				const bool local_;
				const std::size_t seq_;
				const cyng::param_map_t master_;	//	[3] master data
				const cyng::param_map_t client_;	//	[4] client data
				evt_client_req_open_connection(std::pair<std::size_t, bool>
					, bool local
					, std::size_t seq
					, cyng::param_map_t		//	master
					, cyng::param_map_t);	//	client
			};

			/**
			 * incoming connection open response
			 */
			struct evt_client_res_open_connection
			{
				const boost::uuids::uuid peer_;	//	[0] peer
				const std::uint64_t seq_;		//	[1] cluster sequence
				const bool success_;			//	[2] success
				const cyng::param_map_t master_;	//	[3] master data
				const cyng::param_map_t client_;	//	[4] client data

				evt_client_res_open_connection(std::tuple<
					boost::uuids::uuid,		//	[0] peer
					std::uint64_t,			//	[1] cluster sequence
					bool,					//	[2] success
					cyng::param_map_t,		//	[3] options
					cyng::param_map_t		//	[4] bag
				>);	
			};

			/**
			 * incoming connection close request
			 */
			struct evt_client_req_close_connection 
			{
				std::size_t const tsk_;
				bool const success_;
				std::uint64_t const seq_;			//	[1] cluster sequence
				boost::uuids::uuid const tag_;		//	[2] origin-tag - compare to "origin-tag"
				bool const shutdown_;				//	[3] shutdown flag
				cyng::param_map_t const master_;	//	[4] master
				cyng::param_map_t const client_;	//	[5] client
				evt_client_req_close_connection(std::pair<std::size_t, bool>,
					std::uint64_t,			//	[1] cluster sequence
					boost::uuids::uuid,		//	[2] origin-tag - compare to "origin-tag"
					bool,					//	[3] shutdown flag
					cyng::param_map_t,		//	[4] master
					cyng::param_map_t		//	[5] client
				);
			};

			/**
			 * incoming connection close response
			 */
			struct evt_client_res_close_connection 
			{
				const boost::uuids::uuid peer_;	//	[0] peer
				const std::uint64_t seq_;		//	[1] cluster sequence
				const bool success_;			//	[2] success
				const cyng::param_map_t master_;	//	[3] master data
				const cyng::param_map_t client_;	//	[4] client data

				evt_client_res_close_connection(std::tuple<
						boost::uuids::uuid,		//	[0] peer
						std::uint64_t,			//	[1] cluster sequence
						bool,					//	[2] success flag from remote session
						cyng::param_map_t,		//	[3] master data
						cyng::param_map_t		//	[4] client data
					>);		
			};

			/**
			 * communicate with a task
			 */
			struct evt_redirect
			{
				evt_redirect();
			};

			struct evt_shutdown
			{
				evt_shutdown();
			};

			struct evt_data
			{
				cyng::object obj_;
				evt_data(cyng::object);
			};

			struct evt_activity
			{
				evt_activity();
			};

			struct evt_watchdog_started
			{
				const std::size_t tsk_;
				const bool success_;
				evt_watchdog_started(std::pair<std::size_t, bool>);
			};

			struct evt_proxy_started
			{
				const std::size_t tsk_;
				const bool success_;
				evt_proxy_started(std::pair<std::size_t, bool>);
			};

			struct evt_proxy
			{
				evt_proxy(cyng::tuple_t&&);
				cyng::tuple_t tpl_;
			};

			struct evt_sml_msg
			{
				std::size_t const idx_;
				cyng::tuple_t msg_;
				evt_sml_msg(std::size_t, cyng::tuple_t);
			};

			struct evt_sml_eom
			{
				cyng::tuple_t tpl_;
				evt_sml_eom(cyng::tuple_t);
			};

			struct evt_sml_public_close_response
			{
				cyng::tuple_t tpl_;
				evt_sml_public_close_response(cyng::tuple_t);
			};

			struct evt_sml_get_proc_param_response
			{
				cyng::vector_t const vec_;
				evt_sml_get_proc_param_response(cyng::vector_t&&);
			};

			struct evt_sml_get_list_response
			{
				cyng::vector_t vec_;
				evt_sml_get_list_response(cyng::vector_t);
			};

			struct evt_sml_attention_msg
			{
				cyng::vector_t vec_;
				evt_sml_attention_msg(cyng::vector_t);
			};

			//
			//	states
			//
			struct state_idle
			{
				state_idle();
				void stop(cyng::async::mux&);
				std::size_t tsk_gatekeeper_;
				std::uint16_t watchdog_;	//!< minutes
			};
			struct state_authorized
			{
				state_authorized();
				void stop(cyng::async::mux&);
				void activity(cyng::async::mux&);
				std::size_t tsk_watchdog_;
				std::size_t tsk_proxy_;
			};
			struct state_wait_for_open_response
			{
				state_wait_for_open_response();
				void init(std::size_t, bool, std::uint64_t, cyng::param_map_t, cyng::param_map_t);
				cyng::vector_t establish_local_connection() const;
				boost::uuids::uuid get_origin_tag() const;
				void reset();

				std::size_t tsk_connection_open_;
				enum connection_type {
					E_UNDEF,
					E_LOCAL,
					E_REMOTE,
				} type_;
				std::uint64_t seq_;
				cyng::param_map_t master_params_;
				cyng::param_map_t client_params_;
			};
			struct state_wait_for_close_response
			{
				state_wait_for_close_response();
				void init(std::size_t, boost::uuids::uuid, bool, std::uint64_t, cyng::param_map_t, cyng::param_map_t, bool);
				void reset();
				bool is_connection_local() const;
				void terminate(bus::shared_type, response_type res);

				std::size_t tsk_connection_close_;
				boost::uuids::uuid tag_;	//!< original tag
				std::uint64_t seq_;
				bool shutdown_;
				cyng::param_map_t master_params_;
				cyng::param_map_t client_params_;
			};
			struct state_connected_local
			{
				state_connected_local();
				void transmit(bus::shared_type, boost::uuids::uuid tag, cyng::object);
			};
			struct state_connected_remote
			{
				state_connected_remote();
				void transmit(bus::shared_type, boost::uuids::uuid tag, cyng::object);
			};

			struct state_connected_task
			{
				state_connected_task();
				void reset(cyng::async::mux&, std::size_t tsk);
				void transmit(cyng::async::mux&, cyng::object);

				/**
				 * List of visible or active devices/meters
				 */
				void get_proc_parameter(cyng::async::mux&, cyng::vector_t);
				void get_list_response(cyng::async::mux&, cyng::vector_t);
				void attention_msg(cyng::async::mux&, cyng::vector_t);

				/**
				 * same as in state_authorized
				 */
				std::size_t tsk_proxy_;
			};

		}

		class session_state
		{
		public:
			session_state(session*);

			void react(state::evt_init_complete);
			void react(state::evt_shutdown);
			void react(state::evt_activity);
			void react(state::evt_proxy);
			void react(state::evt_data);
			void react(state::evt_watchdog_started);
			void react(state::evt_proxy_started);
			cyng::vector_t react(state::evt_req_login_public);
			cyng::vector_t react(state::evt_req_login_scrambled);
			cyng::vector_t react(state::evt_res_login);
			cyng::vector_t react(state::evt_client_req_open_connection);
			cyng::vector_t react(state::evt_client_res_close_connection);
			cyng::vector_t react(state::evt_client_res_open_connection);
			cyng::vector_t react(state::evt_ipt_req_close_connection);
			cyng::vector_t react(state::evt_ipt_res_open_connection);
			cyng::vector_t react(state::evt_ipt_res_close_connection);
			cyng::vector_t react(state::evt_client_req_close_connection);

			void react(state::evt_sml_msg);
			void react(state::evt_sml_eom);
			void react(state::evt_sml_public_close_response);
			void react(state::evt_sml_get_list_response);
			void react(state::evt_sml_attention_msg);

			void react(state::evt_sml_get_proc_param_response);

			/**
			 * stream connection state as text
			 */
			friend std::ostream& operator<<(std::ostream& os, session_state const& st);

		private:
			void signal_wrong_state(std::string);
			void redirect(cyng::context& ctx);

			cyng::vector_t query(std::uint32_t);

		private:
			session* sp_;
			cyng::logging::log_ptr logger_;
			enum internal_state {
				S_IDLE,	//	TCP/IP connection accepted
				S_ERROR,
				S_SHUTDOWN,
				S_AUTHORIZED,
				S_WAIT_FOR_OPEN_RESPONSE,
				S_WAIT_FOR_CLOSE_RESPONSE,
				S_CONNECTED_LOCAL,
				S_CONNECTED_REMOTE,
				S_CONNECTED_TASK,
			} state_;
			void transit(internal_state);

			state::state_idle	idle_;
			state::state_authorized	authorized_;
			state::state_wait_for_open_response wait_for_open_response_;
			state::state_wait_for_close_response wait_for_close_response_;
			state::state_connected_local local_;
			state::state_connected_remote remote_;
			state::state_connected_task	task_;
		};

	}
}

#endif

