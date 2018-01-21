/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/ipt/header.h>
#include <smf/ipt/codes.h>
#include <boost/assert.hpp>

namespace node
{
	namespace ipt	
	{
		/**
		 *	Default constructor
		 */
		//header::header()
		//{
		//	reset();
		//}

		void header::reset(sequence_type seq)
		{
			command_ = 0x7fff;
			sequence_ = seq;
			reserved_ = 0;
			length_ = HEADER_SIZE;
		}

		std::size_t size(header const& h)
		{
			BOOST_ASSERT(h.length_ >= HEADER_SIZE);
			return (h.length_ < HEADER_SIZE)
				? HEADER_SIZE
				: (h.length_ - HEADER_SIZE)
				;
		}

		bool is_request(command_type cmd)
		{
			return ((cmd & 0x8000) == 0x8000);
		}

		bool is_response(command_type cmd)
		{
			return !is_request(cmd);
		}

		bool is_transport_layer(command_type cmd)
		{
			return (is_custom_layer(cmd))
				? false
				: ((cmd & 0x1000) == 0x1000)
				;
		}

		bool is_application_layer(command_type cmd)
		{
			return (is_custom_layer(cmd))
				? false
				: ((cmd & 0x2000) == 0x2000)
				;
		}

		bool is_control_layer(command_type cmd)
		{
			return (is_custom_layer(cmd))
				? false
				: ((cmd & 0x4000) == 0x4000)
				;
		}

		//	Test bit 13, 14 and 15
		bool is_custom_layer(command_type cmd)
		{
			return ((cmd & 0x7000) == 0x7000);
		}

		std::uint16_t arity(command_type cmd)
		{
			switch (cmd)
			{
			case code::CTRL_RES_LOGIN_PUBLIC:	return 3;	//	login response
			case code::APP_RES_DEVICE_AUTHENTIFICATION:	return 4;	//	authentification response
			case code::APP_REQ_PUSH_TARGET_NAMELIST:	return 5;	//	push target namelist request
			case code::APP_RES_PUSH_TARGET_NAMELIST:	return 2;	//	push target namelist respone - variable!
			case code::APP_RES_PROTOCOL_VERSION:	return 1;	//	protocol version response
			case code::APP_RES_SOFTWARE_VERSION:	return 1;	//	software version response
			case code::APP_RES_DEVICE_IDENTIFIER:	return 1;	//	device identifier response
			case code::APP_RES_NETWORK_STATUS:	return 8;	//	network status response
			case code::APP_RES_IP_STATISTICS:	return 2;	//	ip statistics response
			case code::APP_RES_DEVICE_TIME:	return 1;	//	device time response
			case code::TP_REQ_OPEN_PUSH_CHANNEL:	return 6;	//	push channel open request
			case code::TP_REQ_CLOSE_PUSH_CHANNEL:	return 1;	//	push channel close request
			case code::TP_REQ_PUSHDATA_TRANSFER:	return 5;	//	push data transfer request
			case code::TP_RES_OPEN_PUSH_CHANNEL:	return 7;	//	push channel open response
			case code::TP_RES_CLOSE_PUSH_CHANNEL:	return 2;	//	push channel close response
			case code::TP_RES_PUSHDATA_TRANSFER:	return 5;	//	push data tarnsfer response
			case code::TP_REQ_OPEN_CONNECTION:	return 1;	//	connection open request
			case code::TP_RES_OPEN_CONNECTION:	return 1;	//	connection open response
			case code::TP_RES_CLOSE_CONNECTION:	return 1;	//	connection close request
			case code::CTRL_REQ_LOGOUT:	return 1;	//	logout request - deprecated
			case code::CTRL_RES_LOGOUT:	return 1;	//	logout response - deprecated
			case code::CTRL_RES_REGISTER_TARGET:	return 2;	//	push target register response
			case code::CTRL_RES_DEREGISTER_TARGET:	return 2;	//	push target deregister response
			case code::CTRL_REQ_LOGIN_PUBLIC:	return 2;	//	public login request
			case code::CTRL_REQ_LOGIN_SCRAMBLED:	return 2;	//	scrambled login request (original 3 parameters, one parameter is handled by parser internally)
			case code::CTRL_REQ_REGISTER_TARGET:	return 3;	//	push target register request
			case code::CTRL_REQ_DEREGISTER_TARGET:	return 1;	//	push target deregister request

			default:
				break;
			}
			return 0;
		}

	}	//	ipt	
}	//	node
