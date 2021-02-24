/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_CODES_H
#define SMF_IPT_CODES_H

#include <smf/ipt.h>
#include <cstdint>

namespace smf {

	namespace ipt	{

		/**
		 *	Definition of all IP-T processor instructions
		 */
		enum class code : command_t {

			//	transport
			//	transport - push channel open
			TP_REQ_OPEN_PUSH_CHANNEL = 0x9000,
			TP_RES_OPEN_PUSH_CHANNEL = 0x1000,	//!<	response

			//	transport - push channel close
			TP_REQ_CLOSE_PUSH_CHANNEL = 0x9001,	//!<	request
			TP_RES_CLOSE_PUSH_CHANNEL = 0x1001,	//!<	response

			//	transport - push channel data transfer
			TP_REQ_PUSHDATA_TRANSFER = 0x9002,	//!<	request
			TP_RES_PUSHDATA_TRANSFER = 0x1002,	//!<	response

			//	transport - connection open
			TP_REQ_OPEN_CONNECTION = 0x9003,	//!<	request
			TP_RES_OPEN_CONNECTION = 0x1003,	//!<	response

			//	transport - connection close
			TP_REQ_CLOSE_CONNECTION = 0x9004,	//!<	request
			TP_RES_CLOSE_CONNECTION = 0x1004,	//!<	response

			//	open stream channel
			TP_REQ_OPEN_STREAM_CHANNEL = 0x9006,
			TP_RES_OPEN_STREAM_CHANNEL = 0x1006,

			//	close stream channel
			TP_REQ_CLOSE_STREAM_CHANNEL = 0x9007,
			TP_RES_CLOSE_STREAM_CHANNEL = 0x1007,

			//	stream channel data transfer
			TP_REQ_STREAMDATA_TRANSFER = 0x9008,
			TP_RES_STREAMDATA_TRANSFER = 0x1008,

			//	transport - connection data transfer *** non-standard ***
			//	0x900B:	//	request
			//	0x100B:	//	response

			//	application
			//	application - protocol version
			APP_REQ_PROTOCOL_VERSION = 0xA000,	//!<	request
			APP_RES_PROTOCOL_VERSION = 0x2000,	//!<	response

			//	application - device firmware version
			APP_REQ_SOFTWARE_VERSION = 0xA001,	//!<	request
			APP_RES_SOFTWARE_VERSION = 0x2001,	//!<	response

			//	application - device identifier
			APP_REQ_DEVICE_IDENTIFIER = 0xA003,	//!<	request
			APP_RES_DEVICE_IDENTIFIER = 0x2003,

			//	application - network status
			APP_REQ_NETWORK_STATUS = 0xA004,	//!<	request
			APP_RES_NETWORK_STATUS = 0x2004,	//!<	response

			//	application - IP statistic
			APP_REQ_IP_STATISTICS = 0xA005,	//!<	request
			APP_RES_IP_STATISTICS = 0x2005,	//!<	response

			//	application - device authentification
			APP_REQ_DEVICE_AUTHENTIFICATION = 0xA006,	//!<	request
			APP_RES_DEVICE_AUTHENTIFICATION = 0x2006,	//!<	response

			//	application - device time
			APP_REQ_DEVICE_TIME = 0xA007,	//!<	request
			APP_RES_DEVICE_TIME = 0x2007,	//!<	response

			//	application - push-target namelist
			APP_REQ_PUSH_TARGET_NAMELIST = 0xA008,	//!<	request
			APP_RES_PUSH_TARGET_NAMELIST = 0x2008,	//!<	response

			//	application - push-target echo
			APP_REQ_PUSH_TARGET_ECHO = 0xA009,	//!<	*** deprecated ***
			APP_RES_PUSH_TARGET_ECHO = 0x2009,	//!<	*** deprecated ***

			//	application - traceroute
			APP_REQ_TRACEROUTE = 0xA00A,	//!<	*** deprecated ***
			APP_RES_TRACEROUTE = 0x200A,	//!<	*** deprecated ***

			//	control
			CTRL_RES_LOGIN_PUBLIC = 0x4001,	//!<	public login response
			CTRL_RES_LOGIN_SCRAMBLED = 0x4002,	//!<	scrambled login response
			CTRL_REQ_LOGIN_PUBLIC = 0xC001,	//!<	public login request
			CTRL_REQ_LOGIN_SCRAMBLED = 0xC002,	//!<	scrambled login request

			//	control - maintenance
			MAINTENANCE_REQUEST = 0xC003,	//!<	request *** deprecated ***
			MAINTENANCE_RESPONSE = 0x4003,	//!<	response *** deprecated ***

			//	control - logout
			CTRL_REQ_LOGOUT = 0xC004,	//!<	request *** deprecated ***
			CTRL_RES_LOGOUT = 0x4004,	//!<	response *** deprecated ***

			//	control - push target register
			CTRL_REQ_REGISTER_TARGET = 0xC005,	//!<	request
			CTRL_RES_REGISTER_TARGET = 0x4005,	//!<	response

			//	control - push target deregister
			CTRL_REQ_DEREGISTER_TARGET = 0xC006,	//!<	request
			CTRL_RES_DEREGISTER_TARGET = 0x4006,	//!<	response

			//	control - watchdog
			CTRL_REQ_WATCHDOG = 0xC008,	//!<	request
			CTRL_RES_WATCHDOG = 0x4008,	//!<	response

			//	control - multi public login request
			MULTI_CTRL_REQ_LOGIN_PUBLIC = 0xC009,	//!<	request
			MULTI_CTRL_RES_LOGIN_PUBLIC = 0x4009,	//!<	response

			//	control - multi public login request
			MULTI_CTRL_REQ_LOGIN_SCRAMBLED = 0xC00A,	//!<	request
			MULTI_CTRL_RES_LOGIN_SCRAMBLED = 0x400A,	//!<	response

			//	stream source register
			CTRL_REQ_REGISTER_STREAM_SOURCE = 0xC00B,
			CTRL_RES_REGISTER_STREAM_SOURCE = 0x400B,

			//	stream source deregister
			CTRL_REQ_DEREGISTER_STREAM_SOURCE = 0xC00C,
			CTRL_RES_DEREGISTER_STREAM_SOURCE = 0x400C,

			//	server mode
			SERVER_MODE_REQUEST = 0xC010,	//!<	request
			SERVER_MODE_RESPONSE = 0x4010,	//!<	response

			//	server mode reconnect
			SERVER_MODE_RECONNECT_REQUEST = 0xC011,	//!<	request
			SERVER_MODE_RECONNECT_RESPONSE = 0x4011,	//!<	response


			UNKNOWN = 0x7fff,	//!<	unknown command

		};

		/**
		 *	@return name of the IP-T command
		 */
		const char* command_name(std::uint16_t);

	}	//	ipt
}	

#endif
