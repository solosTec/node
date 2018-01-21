/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_RESPONSE_HPP
#define NODE_IPT_RESPONSE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <smf/ipt/defs.h>
#include <smf/ipt/codes.h>

namespace node
{
	namespace ipt	
	{

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
			explicit response(command_type cmd, sequence_type seq, response_type r);

			/**
			 * Copy constructor
			 */
			response(response const&);

			/**
			 * Assignment
			 */
			response& operator=(response const&);

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
			command_type	get_command() const;

			/**
			 * @return sequence number
			 */
			sequence_type	get_sequence() const;

			/**
			 * @return value of return code
			 */
			response_type	get_code() const;

		private:
			command_type	command_;	//!<	command code
			sequence_type	sequence_;	//!< sequence number 
			response_type	code_;	//!<	response code
		};

		//  factories
		/**
		 * Login response has a sequence number of 0 by definition 
		 */
		response make_login_response(response_type);
		response make_ctrl_res_logout(sequence_type, response_type);
		response make_maintenance_response(sequence_type, response_type);
		response make_tp_res_open_push_channel(sequence_type, response_type);
		response make_tp_res_close_push_channel(sequence_type, response_type);
		response make_tp_res_pushdata_transfer(sequence_type, response_type);
		response make_push_ctrl_res_register_target(sequence_type, response_type);
		response make_push_ctrl_res_deregister_target(sequence_type, response_type);
		response make_tp_res_open_connection(sequence_type, response_type);
		response make_tp_res_close_connection(sequence_type, response_type);
		response make_app_res_network_status(sequence_type, response_type);
		response make_app_res_ip_statistics(sequence_type, response_type);
		response make_app_res_push_target_namelist(sequence_type, response_type);
		response make_multiple_login_response(sequence_type, response_type);

		namespace detail	
		{

			template < code::ipt_op_code_enum >
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
				enum Response : response_type
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
					MALFUNCTION,		//!<	faulty master
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
				enum Response : response_type
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
				enum Response : response_type
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
				enum Response : response_type
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
				enum Response : response_type
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
				enum Response : response_type
				{
					DIALUP_FAILED,		//!<	Connection establishment failed
					DIALUP_SUCCESS,		//!<	Connection successfully established
					BUSY,				//!<	The line is busy
					NO_MASTER,			//!<	No connection to master
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
				enum Response : response_type
				{
					CONNECTION_CLEARING_FAILED,			//!<	removed since April 2008
					CONNECTION_CLEARING_SUCCEEDED,
					CONNECTION_CLEARING_FORBIDDEN,		//!<	inserted since April 2008 - for leased lines
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

			/**	@brief network status response (0x2004).
			 *
			 *	Acknowledge network status request.
			 */
			template <>
			struct response_policy< code::APP_RES_NETWORK_STATUS >
			{
				enum DeviceType
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
				enum Response : response_type
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
				enum Response : response_type
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
				enum Response : response_type
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
				enum Response : response_type
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
				enum status_enum : response_type
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
				enum Response : response_type
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
					MALFUNCTION,		//!<	faulty master
				};

				static bool is_success(unsigned);
				static const char* get_response_name(unsigned);
			};

		}	//	detail

		//	map into namespace node::ipt
		typedef detail::response_policy< code::CTRL_RES_LOGIN_PUBLIC >	ctrl_res_login_public_policy;
		typedef detail::response_policy< code::CTRL_RES_LOGOUT >	public_CTRL_RES_LOGOUT_policy;
		typedef detail::response_policy< code::MAINTENANCE_RESPONSE >	maintenance_response_policy;
		typedef detail::response_policy< code::TP_RES_OPEN_PUSH_CHANNEL >	tp_res_open_push_channel_policy;
		typedef detail::response_policy< code::TP_RES_CLOSE_PUSH_CHANNEL >	tp_res_close_push_channel_policy;
		typedef detail::response_policy< code::TP_RES_OPEN_CONNECTION >	TP_RES_OPEN_CONNECTION_policy;
		typedef detail::response_policy< code::TP_RES_CLOSE_CONNECTION >	TP_RES_CLOSE_CONNECTION_policy;
		typedef detail::response_policy< code::APP_RES_NETWORK_STATUS >	app_res_network_status_policy;
		typedef detail::response_policy< code::APP_RES_IP_STATISTICS >	ip_statistics_response_policy;
		typedef detail::response_policy< code::CTRL_RES_REGISTER_TARGET >	CTRL_RES_REGISTER_TARGET_policy;
		typedef detail::response_policy< code::CTRL_RES_DEREGISTER_TARGET >	CTRL_RES_DEREGISTER_TARGET_policy;
		typedef detail::response_policy< code::TP_RES_PUSHDATA_TRANSFER >	TP_RES_PUSHDATA_TRANSFER_policy;
		typedef detail::response_policy< code::APP_RES_PUSH_TARGET_NAMELIST >	APP_RES_PUSH_TARGET_NAMELIST_policy;
		typedef detail::response_policy< code::MULTI_CTRL_RES_LOGIN_PUBLIC >	multiple_CTRL_RES_LOGIN_PUBLIC_policy;

	}	//	ipt
}	//	node


#endif	//	NODDY_IPT_RESPONSE_HPP
