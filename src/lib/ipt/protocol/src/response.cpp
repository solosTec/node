/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/response.hpp>

namespace smf {

	namespace ipt {

		response::response(code command, sequence_t seq, response_t code)
			: command_(command)
			, sequence_(seq)
			, code_(code)
		{}

		code response::get_command() const	{
			return command_;
		}

		sequence_t response::get_sequence() const {
			return sequence_;
		}

		response_t response::get_code() const {
			return code_;
		}

		bool response::is_success() const
		{
			switch (command_)
			{
			case code::TP_RES_OPEN_PUSH_CHANNEL:
				return detail::response_policy< code::TP_RES_OPEN_PUSH_CHANNEL >::is_success(code_);
			case code::TP_RES_CLOSE_PUSH_CHANNEL:
				return detail::response_policy< code::TP_RES_CLOSE_PUSH_CHANNEL >::is_success(code_);
			case code::TP_RES_PUSHDATA_TRANSFER:
				return detail::response_policy< code::TP_RES_PUSHDATA_TRANSFER >::is_success(code_);
			case code::TP_RES_OPEN_CONNECTION:
				return detail::response_policy< code::TP_RES_OPEN_CONNECTION >::is_success(code_);
			case code::TP_RES_CLOSE_CONNECTION:
				return detail::response_policy< code::TP_RES_CLOSE_CONNECTION >::is_success(code_);

			case code::APP_RES_IP_STATISTICS:
				return detail::response_policy< code::APP_RES_IP_STATISTICS >::is_success(code_);
			case code::APP_RES_PUSH_TARGET_NAMELIST:
				return detail::response_policy< code::APP_RES_PUSH_TARGET_NAMELIST >::is_success(code_);

			case code::CTRL_RES_LOGIN_PUBLIC:
				return detail::response_policy< code::CTRL_RES_LOGIN_PUBLIC >::is_success(code_);
			case code::CTRL_RES_LOGOUT:
				return detail::response_policy< code::CTRL_RES_LOGOUT >::is_success(code_);
			case code::MAINTENANCE_RESPONSE:
				return detail::response_policy< code::MAINTENANCE_RESPONSE >::is_success(code_);
			case code::CTRL_RES_REGISTER_TARGET:
				return detail::response_policy< code::CTRL_RES_REGISTER_TARGET >::is_success(code_);
			case code::CTRL_RES_DEREGISTER_TARGET:
				return detail::response_policy< code::CTRL_RES_DEREGISTER_TARGET >::is_success(code_);
			default:
				break;
			}
			return false;
		}
		
		response::operator bool() const
		{
			return is_success();
		}
		

		const char* response::get_response_name() const
		{
			switch (command_)
			{
			case code::TP_RES_OPEN_PUSH_CHANNEL:
				return detail::response_policy< code::TP_RES_OPEN_PUSH_CHANNEL >::get_response_name(code_);
			case code::TP_RES_CLOSE_PUSH_CHANNEL:
				return detail::response_policy< code::TP_RES_CLOSE_PUSH_CHANNEL >::get_response_name(code_);
			case code::TP_RES_PUSHDATA_TRANSFER:
				return detail::response_policy< code::TP_RES_PUSHDATA_TRANSFER >::get_response_name(code_);
			case code::TP_RES_OPEN_CONNECTION:
				return detail::response_policy< code::TP_RES_OPEN_CONNECTION >::get_response_name(code_);
			case code::TP_RES_CLOSE_CONNECTION:
				return detail::response_policy< code::TP_RES_CLOSE_CONNECTION >::get_response_name(code_);

			case code::APP_RES_IP_STATISTICS:
				return detail::response_policy< code::APP_RES_IP_STATISTICS >::get_response_name(code_);
			case code::APP_RES_PUSH_TARGET_NAMELIST:
				return detail::response_policy< code::APP_RES_PUSH_TARGET_NAMELIST >::get_response_name(code_);

			case code::CTRL_RES_LOGIN_PUBLIC:
				return detail::response_policy< code::CTRL_RES_LOGIN_PUBLIC >::get_response_name(code_);
			case code::CTRL_RES_LOGOUT:
				return detail::response_policy< code::CTRL_RES_LOGOUT >::get_response_name(code_);
			case code::CTRL_RES_LOGIN_SCRAMBLED:
				return detail::response_policy< code::CTRL_RES_LOGIN_PUBLIC >::get_response_name(code_);
			case code::MAINTENANCE_RESPONSE:
				return detail::response_policy< code::MAINTENANCE_RESPONSE >::get_response_name(code_);
			case code::CTRL_RES_REGISTER_TARGET:
				return detail::response_policy< code::CTRL_RES_REGISTER_TARGET >::get_response_name(code_);
			case code::CTRL_RES_DEREGISTER_TARGET:
				return detail::response_policy< code::CTRL_RES_DEREGISTER_TARGET >::get_response_name(code_);
			default:
				break;
			}
			return "";
		}


		//  factory
		response make_login_response(response_t code)
		{
			return response(code::CTRL_RES_LOGIN_PUBLIC, 0, code);
		}

		response make_ctrl_res_logout(sequence_t seq, response_t code)
		{
			return response(code::CTRL_RES_LOGOUT, seq, code);
		}

		response make_maintenance_response(sequence_t seq, response_t code)
		{
			return response(code::MAINTENANCE_RESPONSE, seq, code);
		}
		response make_tp_res_open_push_channel(sequence_t seq, response_t code)
		{
			return response(code::TP_RES_OPEN_PUSH_CHANNEL, seq, code);
		}

		response make_tp_res_close_push_channel(sequence_t seq, response_t code)
		{
			return response(code::TP_RES_CLOSE_PUSH_CHANNEL, seq, code);
		}

		response make_tp_res_pushdata_transfer(sequence_t seq, response_t code)
		{
			return response(code::TP_RES_PUSHDATA_TRANSFER, seq, code);
		}

		response make_push_ctrl_res_register_target(sequence_t seq, response_t code)
		{
			return response(code::CTRL_RES_REGISTER_TARGET, seq, code);
		}

		response make_push_ctrl_res_deregister_target(sequence_t seq, response_t code)
		{
			return response(code::CTRL_RES_DEREGISTER_TARGET, seq, code);
		}

		response make_tp_res_open_connection(sequence_t seq, response_t code)
		{
			return response(code::TP_RES_OPEN_CONNECTION, seq, code);
		}

		response make_tp_res_close_connection(sequence_t seq, response_t code)
		{
			return response(code::TP_RES_CLOSE_CONNECTION, seq, code);
		}

		response make_app_res_network_status(sequence_t seq, response_t code)
		{
			return response(code::APP_RES_NETWORK_STATUS, seq, code);
		}

		response make_app_res_ip_statistics(sequence_t seq, response_t code)
		{
			return response(code::APP_RES_IP_STATISTICS, seq, code);
		}

		response make_app_res_push_target_namelist(sequence_t seq, response_t code)
		{
			return response(code::APP_RES_PUSH_TARGET_NAMELIST, seq, code);
		}

		response make_multiple_login_response(sequence_t seq, response_t code)
		{
			return response(code::MULTI_CTRL_RES_LOGIN_PUBLIC, seq, code);
		}

		namespace detail
		{

			//	+-----------------------------------------------------------------+
			//	| gex::ipt::login_response [definition]
			//	+-----------------------------------------------------------------+
			/**
			*	It's possible to login into a locked account succesfully.
			*	But all further actions are stalled.
			*
			*	@return <code>true<code> if response code is SUCCESS or ACCOUNT_LOCKED.
			*/
			bool response_policy< code::CTRL_RES_LOGIN_PUBLIC >::is_success(unsigned response)
			{
				return response == SUCCESS
					|| response == ACCOUNT_LOCKED;
			}

			bool response_policy< code::CTRL_RES_LOGIN_PUBLIC >::is_locked(unsigned response)
			{
				return response == ACCOUNT_LOCKED;
			}

			bool response_policy< code::CTRL_RES_LOGIN_PUBLIC >::is_redirect(unsigned response)
			{
				return response == NEW_ADDRESS;
			}

			const char* response_policy< code::CTRL_RES_LOGIN_PUBLIC >::get_response_name(unsigned response)
			{
				switch (response)
				{
				case GENERAL_ERROR:
					return "unassigned login error (GENERAL_ERROR)";
				case SUCCESS:
					return "successful login (SUCCESS)";
				case UNKNOWN_ACCOUNT:
					return "unknown account name (UNKNOWN_ACCOUNT)";
				case WRONG_PASSWORD:
					return "wrong password (WRONG_PASSWORD)";
				case ALREADY_LOGGED_ON:
					return "account already in use (ALREADY_LOGGED_ON)";
				case NEW_ADDRESS:
					return "request for re-login with new address (NEW_ADDRESS)";

				case RESERVED_06:
					return "RESERVED_06";
				case RESERVED_07:
					return "RESERVED_07";
				case RESERVED_08:
					return "RESERVED_08";
				case RESERVED_09:
					return "RESERVED_09";
				case RESERVED_0A:
					return "RESERVED_0A";
				case RESERVED_0B:
					return "RESERVED_0B";
				case RESERVED_0C:
					return "RESERVED_0C";

				case ACCOUNT_LOCKED:
					return "account is disabled (ACCOUNT_LOCKED)";

				case MALFUNCTION:
					return "faulty master (MALFUNCTION)";

				default:
					break;
				}
				return "illegal value";
			}

			//	+-----------------------------------------------------------------+
			//	| gex::ipt::CTRL_REQ_LOGOUT [definition]
			//	+-----------------------------------------------------------------+
			//bool response_policy< code::CTRL_REQ_LOGOUT >::is_success( unsigned response )
			//{
			//	return response == NORMAL;
			//}
			//
			//
			//const char* response_policy< code::CTRL_REQ_LOGOUT >::get_response_name( unsigned response )
			//{
			//	switch (response)
			//	{
			//	case GENERAL_ERROR:
			//		return "logout error (GENERAL_ERROR)";
			//	case NORMAL:
			//		return "successful logout (SUCCESS)";
			//	default:
			//		break;
			//	}
			//	return "illegal value";
			//}

			//	+-----------------------------------------------------------------+
			//	| gex::ipt::CTRL_RES_LOGOUT [definition]
			//	+-----------------------------------------------------------------+
			bool response_policy< code::CTRL_RES_LOGOUT >::is_success(unsigned response)
			{
				return response == NORMAL;
			}


			const char* response_policy< code::CTRL_RES_LOGOUT >::get_response_name(unsigned response)
			{
				switch (response)
				{
				case LOGOUT_ERROR:
					return "logout error (LOGOUT_ERROR)";
				case NORMAL:
					return "successful logout (SUCCESS)";
				default:
					break;
				}
				return "illegal value";
			}


			//	+-----------------------------------------------------------------+
			//	| maintenance_response [definition]
			//	+-----------------------------------------------------------------+
			bool response_policy< code::MAINTENANCE_RESPONSE >::is_success(unsigned response)
			{
				return response == SUCCESS;
			}

			const char* response_policy< code::MAINTENANCE_RESPONSE >::get_response_name(unsigned response)
			{
				switch (response)
				{
				case CANCELLED:	return "CANCELLED";		//	error logout cancelled
				case SUCCESS:	return "SUCCESS";
				}
				return "unknown maintenance response";
			}

			//	+-----------------------------------------------------------------+
			//	| TP_RES_OPEN_PUSH_CHANNEL [definition]
			//	+-----------------------------------------------------------------+

			bool response_policy< code::TP_RES_OPEN_PUSH_CHANNEL>::is_success(unsigned response)
			{
				return response == SUCCESS;
			}

			const char*
				response_policy< code::TP_RES_OPEN_PUSH_CHANNEL>::get_response_name(unsigned response)
			{
					switch (response)
					{
					case UNREACHABLE:	return "UNREACHABLE";
					case SUCCESS:		return "SUCCESS";
					case UNDEFINED:		return "UNDEFINED";
					case ALREADY_OPEN:	return "ALREADY OPEN";
					default:
						break;
					}
					return "unknown push channel open response";
			}

			//	+-----------------------------------------------------------------+
			//	| TP_RES_CLOSE_PUSH_CHANNEL [definition]
			//	+-----------------------------------------------------------------+
			bool response_policy< code::TP_RES_CLOSE_PUSH_CHANNEL>::is_success(unsigned response)
			{
				return response == SUCCESS;
			}

			const char*
				response_policy< code::TP_RES_CLOSE_PUSH_CHANNEL>::get_response_name(unsigned response)
			{
					switch (response)
					{
					case UNREACHABLE:	return "UNREACHABLE";
					case SUCCESS:		return "SUCCESS";
					case UNDEFINED:		return "UNDEFINED";
					case BROKEN:		return "BROKEN";
					}
					return "unknown push channel close response";
			}

			//	+-----------------------------------------------------------------+
			//	| TP_RES_OPEN_CONNECTION [definition]
			//	+-----------------------------------------------------------------+
			bool response_policy< code::TP_RES_OPEN_CONNECTION >::is_success(unsigned r)
			{
				return r == DIALUP_SUCCESS;
			}

			const char*
				response_policy< code::TP_RES_OPEN_CONNECTION >::get_response_name(unsigned r)
			{
					switch (r)
					{
					case DIALUP_FAILED:		return "Connection establishment failed";
					case DIALUP_SUCCESS:	return "Connection successful established";
					case BUSY:				return "The line is busy";
					case NO_MASTER:			return "No connection to primary";
					case UNREACHABLE:		return "Remote station unreachable";
					default:
						break;
					}
					return "unknown connection open response";
			}

			//	+-----------------------------------------------------------------+
			//	| TP_RES_CLOSE_CONNECTION [definition]
			//	+-----------------------------------------------------------------+
			bool response_policy< code::TP_RES_CLOSE_CONNECTION >::is_success(unsigned r)
			{
				return r == CONNECTION_CLEARING_SUCCEEDED;
			}
			const char* response_policy< code::TP_RES_CLOSE_CONNECTION >::get_response_name(unsigned r)
			{
				switch (r)
				{
				case CONNECTION_CLEARING_FAILED:
					return "Connection clearing failed";
				case CONNECTION_CLEARING_SUCCEEDED:
					return "Connection clearing successful";
				case CONNECTION_CLEARING_FORBIDDEN:
					return "Connection clearing forbidden";
				default:
					break;
				}
				return "unknown connection close response";
			}

			//	+-----------------------------------------------------------------+
			//	| TP_RES_OPEN_STREAM_CHANNEL [definition]
			//	+-----------------------------------------------------------------+
			bool response_policy< code::TP_RES_OPEN_STREAM_CHANNEL >::is_success(unsigned r)
			{
				return r == SUCCESS;
			}
			const char* response_policy< code::TP_RES_OPEN_STREAM_CHANNEL >::get_response_name(unsigned r)
			{
				switch (r)
				{
				case GENERAL_ERROR:	return "stream source cannot be reached";
				case SUCCESS:		return "success";
				case NOT_REGISTERED:	return "stream source not registered";
				default:
					break;
				}
				return "unknown open stream channel response";

			}

			//	+-----------------------------------------------------------------+
			//	| APP_RES_IP_STATISTICS [definition]
			//	+-----------------------------------------------------------------+

			bool response_policy< code::APP_RES_IP_STATISTICS >::is_success(unsigned r)
			{
				return r == ACCEPTED;
			}

			const char*
				response_policy< code::APP_RES_IP_STATISTICS >::get_response_name(unsigned r)
			{
					switch (r)
					{
					case NOT_APPLICABLE:
						return "NOT APPLICABLE";
					case ACCEPTED:
						return "ACCEPTED";
					default:
						break;
					}
					return "unknown ip statistic response";
			}

			//	+-----------------------------------------------------------------+
			//	| push_CTRL_RES_REGISTER_TARGET [definition]
			//	+-----------------------------------------------------------------+
			bool response_policy< code::CTRL_RES_REGISTER_TARGET >::is_success(unsigned response)
			{
				return response == OK;
			}

			const char*
				response_policy< code::CTRL_RES_REGISTER_TARGET >::get_response_name(unsigned response)
			{
					switch (response)
					{
					case GENERAL_ERROR:
						return "registration denied (GENERAL_ERROR)";
					case OK:
						return "OK";
					case REJECTED:
						return "duplicate push target (REJECTED)";
					default:
						break;
					}
					return "unknown push target register response";
			}


			//	+-----------------------------------------------------------------+
			//	| push_CTRL_RES_DEREGISTER_TARGET [definition]
			//	+-----------------------------------------------------------------+
			bool response_policy< code::CTRL_RES_DEREGISTER_TARGET >::is_success(unsigned response)
			{
				return response == OK;
			}

			const char*
				response_policy< code::CTRL_RES_DEREGISTER_TARGET >::get_response_name(unsigned response)
			{
					switch (response)
					{
					case GENERAL_ERROR:
						return "deregistration failed (GENERAL_ERROR)";
					case OK:
						return "OK";
					default:
						break;
					}
					return "unknown push target deregister response";
			}

			//	+-----------------------------------------------------------------+
			//	| TP_RES_PUSHDATA_TRANSFER [definition]
			//	+-----------------------------------------------------------------+
			bool response_policy< code::TP_RES_PUSHDATA_TRANSFER >::is_success(unsigned response)
			{
				return response == SUCCESS;
			}

			const char*
				response_policy< code::TP_RES_PUSHDATA_TRANSFER >::get_response_name(unsigned response)
			{
					switch (response)
					{
					case UNREACHABLE:
						return "UNREACHABLE";
					case SUCCESS:
						return "SUCCESS";
					case UNDEFINED:
						return "UNDEFINED";
					case BROKEN:
						return "BROKEN";
					}
					return "unknown push data transfer response";
			}

			//	+-----------------------------------------------------------------+
			//	| APP_RES_PUSH_TARGET_NAMELIST [definition]
			//	+-----------------------------------------------------------------+
			bool response_policy< code::APP_RES_PUSH_TARGET_NAMELIST >::is_success(unsigned response)
			{
				return response == OK;
			}

			//static
			const char*
				response_policy< code::APP_RES_PUSH_TARGET_NAMELIST >::get_response_name(unsigned response)
			{
					switch (response)
					{
					case GENERAL_ERROR:
						return "GENERAL_ERROR";
					case OK:
						return "OK";
					default:
						break;
					}
					return "unknown push target namelist response";
			}

			//	+-----------------------------------------------------------------+
			//	| multi_login_response_response [definition]
			//	+-----------------------------------------------------------------+
			bool response_policy< code::MULTI_CTRL_RES_LOGIN_PUBLIC >::is_success(unsigned response)
			{
				return ctrl_res_login_public_policy::is_success(response);
			}

			//static
			const char*
				response_policy< code::MULTI_CTRL_RES_LOGIN_PUBLIC >::get_response_name(unsigned response)
			{
				return ctrl_res_login_public_policy::get_response_name(response);
			}

		}	//	detail

	}	//	ipt
}
