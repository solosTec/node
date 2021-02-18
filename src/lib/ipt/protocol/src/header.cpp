/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/header.h>
#include <smf/ipt/codes.h>
#include <cstring>

#include <boost/assert.hpp>

namespace smf
{
	namespace ipt	
	{
		void header::reset(sequence_t seq)
		{
			command_ = 0x7fff;
			sequence_ = seq;
			reserved_ = 0;
			length_ = HEADER_SIZE;
		}

		header to_header(cyng::buffer_t const& src) {

			BOOST_ASSERT(HEADER_SIZE == src.size());

			header h;
			if (HEADER_SIZE == src.size())	std::memcpy(&h, src.data(), src.size());
			else h.reset(INVALID_SEQ);
			return h;
		}


		std::uint16_t arity(command_t cmd)
		{
			switch (static_cast<code>(cmd))
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
}	
