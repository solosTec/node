/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_RESPONSE_HPP
#define SMF_IPT_RESPONSE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <smf/ipt.h>
#include <smf/ipt/codes.h>

namespace smf {

	namespace ipt	{

		/**	@brief Generalized data structure for IP-T responses
		 */
		class response
		{
		public:
			/**
			 * @param cmd IP-T command code
			 * @param seq sequence number
			 * @param r response code
			 */
			explicit response(code cmd, sequence_t seq, response_t r);

			/**
			 * Copy constructor
			 */
			response(response const&) = default;

			/**
			 * Assignment
			 */
			response& operator=(response const&) = default;

			/**
			 * @return true if response code signals no error
			 */
			bool is_success() const;
			explicit operator bool() const;

			/**
			 * @return Textual description (english) of response code
			 */
			const char* get_response_name() const;

			/**
			 * @return value of IP-T command code
			 */
			code get_command() const;

			/**
			 * @return sequence number
			 */
			sequence_t	get_sequence() const;

			/**
			 * @return value of return code
			 */
			response_t	get_code() const;

		private:
			code	command_;	//!<	command code
			sequence_t	sequence_;	//!<	sequence number 
			response_t	code_;		//!<	response code
		};

		//  factories
		/**
		 * Login response has a sequence number of 0 by definition 
		 */
		response make_login_response(response_t);
		response make_ctrl_res_logout(sequence_t, response_t);
		response make_maintenance_response(sequence_t, response_t);
		response make_tp_res_open_push_channel(sequence_t, response_t);
		response make_tp_res_close_push_channel(sequence_t, response_t);
		response make_tp_res_pushdata_transfer(sequence_t, response_t);
		response make_push_ctrl_res_register_target(sequence_t, response_t);
		response make_push_ctrl_res_deregister_target(sequence_t, response_t);
		response make_tp_res_open_connection(sequence_t, response_t);
		response make_tp_res_close_connection(sequence_t, response_t);
		response make_app_res_network_status(sequence_t, response_t);
		response make_app_res_ip_statistics(sequence_t, response_t);
		response make_app_res_push_target_namelist(sequence_t, response_t);
		response make_multiple_login_response(sequence_t, response_t);

		namespace detail	
		{

			template < code >
			struct response_policy
			{};

			/**	@brief login response
			 *
			 *	Acknowledge login request. There is no difference between public
			 *	and scrambled version.
			 */
			template <>
			struct response_policy< code::CTRL_RES_LOGIN_PUBLIC >
			{
				enum Response : response_t
				{
					GENERAL_ERROR,		//!<	unassigned login error
					SUCCESS,			//!<	login successful
					UNKNOWN_ACCOUNT,	//!<	unknown account name
					WRONG_PASSWORD,		//!<	wrong password
					ALREADY_LOGGED_ON,	//!<	account already in use
					NEW_ADDRESS,		//!<	request for relogin with new address

					RESERVED_06,		//!<	diverging authentification data
					RESERVED_07,		//!<	not used yet
					RESERVED_08,		//!<	not used yet
					RESERVED_09,		//!<	not used yet
					RESERVED_0A,		//!<	not used yet
					RESERVED_0B,		//!<	not used yet
					RESERVED_0C,		//!<	not used yet

					ACCOUNT_LOCKED,		//!<	account is blocked
					MALFUNCTION,		//!<	faulty primary
				};

				static bool is_success(unsigned);
				bool is_locked(unsigned);
				bool is_redirect(unsigned);
				static const char* get_response_name(unsigned);
			};


			/**	@brief logout response
			 *
			 *	deprecated
			 */
			template <>
			struct response_policy< code::CTRL_RES_LOGOUT >
			{
				enum Response : response_t
				{
					LOGOUT_ERROR,
					NORMAL,
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

			/**	@brief maintenance_response response (0x4003).
			 *
			 */
			template <>
			struct response_policy< code::MAINTENANCE_RESPONSE >
			{
				enum Response : response_t
				{
					CANCELLED,		//!<	error logout cancelled
					SUCCESS,
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

			/**	@brief push channel open response (0x1000).
			 *
			 *	Acknowledge push channel open request.
			 *
			 *	@note revised standard in june 2008.
			 */
			template <>
			struct response_policy< code::TP_RES_OPEN_PUSH_CHANNEL >
			{
				enum Response : response_t
				{
					UNREACHABLE,
					SUCCESS,
					UNDEFINED,		//!<	no target with the specified prefix found
					ALREADY_OPEN,	//!<	channel already open
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

			/**	@brief push channel close response (0x1001).
			 *
			 *	Acknowledge push channel close request.
			 */
			template <>
			struct response_policy< code::TP_RES_CLOSE_PUSH_CHANNEL >
			{
				enum Response : response_t
				{
					UNREACHABLE,
					SUCCESS,
					UNDEFINED,
					BROKEN,
				};
				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

			/**	@brief connection open response (0x1003).
			 *
			 *	Acknowledge connection open request.
			 */
			template <>
			struct response_policy< code::TP_RES_OPEN_CONNECTION >
			{
				enum Response : response_t
				{
					DIALUP_FAILED,		//!<	Connection establishment failed
					DIALUP_SUCCESS,		//!<	Connection successfuly established
					BUSY,				//!<	The line is busy
					NO_MASTER,			//!<	No connection to primary
					UNREACHABLE,		//!<	Remote station unreachable
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

			/**	@brief connection close response (0x1004).
			 *
			 *	Acknowledge connection close request.
			 */
			template <>
			struct response_policy< code::TP_RES_CLOSE_CONNECTION >
			{
				enum Response : response_t
				{
					CONNECTION_CLEARING_FAILED,			//!<	removed since April 2008
					CONNECTION_CLEARING_SUCCEEDED,
					CONNECTION_CLEARING_FORBIDDEN,		//!<	inserted since April 2008 - for leased lines
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

			/**
			 * open stream channel
			 * TP_RES_OPEN_STREAM_CHANNEL = 0x1006,
			 */
			template <>
			struct response_policy< code::TP_RES_OPEN_STREAM_CHANNEL >
			{
				enum Response : response_t
				{
					GENERAL_ERROR,		//!<	stream source cannot be reached
					SUCCESS,
					NOT_REGISTERED,		//!<	stream source not registered
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

			//	close stream channel
			//TP_RES_CLOSE_STREAM_CHANNEL = 0x1007,

			//	stream channel data transfer
			//TP_RES_STREAMDATA_TRANSFER = 0x1008,

			/**	@brief network status response (0x2004).
			 *
			 *	Acknowledge network status request.
			 */
			template <>
			struct response_policy< code::APP_RES_NETWORK_STATUS >
			{
				enum DeviceType : std::uint8_t
				{
					GPRS_SLAVE,
					LAN_SLAVE,
				};
			};

			/**	@brief ip statistic response (0x2005).
			 *
			 *	Acknowledge IP statistic request.
			 */
			template <>
			struct response_policy< code::APP_RES_IP_STATISTICS >
			{
				enum Response : response_t
				{
					NOT_APPLICABLE,
					ACCEPTED,
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

			/**	@brief push target register response (0x4005).
			 *
			 *	Acknowledge push target register request request.
			 *
			 *	@note introduced in june 2008
			 */
			template <>
			struct response_policy< code::CTRL_RES_REGISTER_TARGET >
			{
				enum Response : response_t
				{
					GENERAL_ERROR,	//!<	general fault
					OK,				//!<	success
					REJECTED,		//!<	registration rejected.
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

			/**	@brief push target deregister response (0x4006).
			 *
			 *	Acknowledge push target de-register request.
			 *
			 *	@note introduced in june 2008
			 */
			template <>
			struct response_policy< code::CTRL_RES_DEREGISTER_TARGET >
			{
				enum Response : response_t
				{
					GENERAL_ERROR,	//!<	general fault
					OK,				//!<	success
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

			/**	@brief push channel transfer response (0x1002).
			 *
			 *	Acknowledge push channel transfer request.
			 */
			template <>
			struct response_policy< code::TP_RES_PUSHDATA_TRANSFER >
			{
				enum Response : response_t
				{
					UNREACHABLE,
					SUCCESS,
					UNDEFINED,
					BROKEN,
				};

				enum status_enum : std::uint8_t
				{
					ACK = 0x20,	//!<	All packets after the initial SYN packet sent by the client should have this flag set.
					SYN = 0x40,	//!<	Only the first packet sent from each end should have this flag set.
					FIN = 0x80,	//!<	No more data from sender.
				};

				enum prio_enum : std::uint8_t
				{
					PRIO_LOW = 0,
					PRIO_NORMAL = 1,
					PRIO_HIGH = 2,
					PRIO_ALERT = 3,
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);

			};

			/**	@brief push target namelist response (0x2008)
			 *
			 *	Acknowledge push target namelist request.
			 *
			 *	@note modified since june 2008
			 */
			template <>
			struct response_policy< code::APP_RES_PUSH_TARGET_NAMELIST >
			{
				enum status_enum : response_t
				{
					GENERAL_ERROR,
					OK,
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

			/**	@brief Multi-public login response (0x4009)
			 *
			 *	Acknowledge Multi-public login request.
			 */
			template <>
			struct response_policy< code::MULTI_CTRL_RES_LOGIN_PUBLIC >
			{
				//	same as public login response
				enum Response : response_t
				{
					GENERAL_ERROR,		//!<	unassigned login error
					SUCCESS,			//!<	login successful
					UNKNOWN_ACCOUNT,	//!<	unknown account name
					WRONG_PASSWORD,		//!<	wrong password
					ALREADY_LOGGED_ON,	//!<	account already in use
					NEW_ADDRESS,		//!<	request for relogin with new address

					RESERVED_06,		//!<	diverging authentification data
					RESERVED_07,		//!<	not used yet
					RESERVED_08,		//!<	not used yet
					RESERVED_09,		//!<	not used yet
					RESERVED_0A,		//!<	not used yet
					RESERVED_0B,		//!<	not used yet
					RESERVED_0C,		//!<	not used yet

					ACCOUNT_LOCKED,		//!<	account is blocked
					MALFUNCTION,		//!<	faulty primary
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

		}	//	detail

		//	map into namespace node::ipt
		using ctrl_res_login_public_policy = detail::response_policy< code::CTRL_RES_LOGIN_PUBLIC >;
		using public_ctrl_res_logout_policy = detail::response_policy< code::CTRL_RES_LOGOUT >;
		using maintenance_response_policy = detail::response_policy< code::MAINTENANCE_RESPONSE >;
		using tp_res_open_push_channel_policy = detail::response_policy< code::TP_RES_OPEN_PUSH_CHANNEL >;
		using tp_res_close_push_channel_policy = detail::response_policy< code::TP_RES_CLOSE_PUSH_CHANNEL >;
		using tp_res_open_connection_policy = detail::response_policy< code::TP_RES_OPEN_CONNECTION >;
		using tp_res_close_connection_policy = detail::response_policy< code::TP_RES_CLOSE_CONNECTION >;
		using app_res_network_status_policy = detail::response_policy< code::APP_RES_NETWORK_STATUS >;
		using ip_statistics_response_policy = detail::response_policy< code::APP_RES_IP_STATISTICS >;
		using ctrl_res_register_target_policy = detail::response_policy< code::CTRL_RES_REGISTER_TARGET >;
		using ctrl_res_deregister_target_policy = detail::response_policy< code::CTRL_RES_DEREGISTER_TARGET >;
		using tp_res_pushdata_transfer_policy = detail::response_policy< code::TP_RES_PUSHDATA_TRANSFER >;
		using app_res_push_target_namelist_policy = detail::response_policy< code::APP_RES_PUSH_TARGET_NAMELIST >;
		using multiple_ctrl_res_login_public_policy = detail::response_policy< code::MULTI_CTRL_RES_LOGIN_PUBLIC >;

	}	//	ipt
}


#endif
