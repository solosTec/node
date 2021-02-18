/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/codes.h>
#include <boost/assert.hpp>

namespace smf {

	namespace ipt	{

		const char* command_name(std::uint16_t c)
		{

			switch (static_cast<code>(c)) {

			case code::TP_REQ_OPEN_PUSH_CHANNEL:	return "TP_REQ_OPEN_PUSH_CHANNEL";
			case code::TP_RES_OPEN_PUSH_CHANNEL:	return "TP_RES_OPEN_PUSH_CHANNEL";

			//	transport - push channel close
			case code::TP_REQ_CLOSE_PUSH_CHANNEL:	return "TP_REQ_CLOSE_PUSH_CHANNEL";
			case code::TP_RES_CLOSE_PUSH_CHANNEL:	return "TP_RES_CLOSE_PUSH_CHANNEL";

			//	transport - push channel data transfer
			case code::TP_REQ_PUSHDATA_TRANSFER:	return "TP_REQ_PUSHDATA_TRANSFER";
			case code::TP_RES_PUSHDATA_TRANSFER:	return "TP_RES_PUSHDATA_TRANSFER";

			//	transport - connection open
			case code::TP_REQ_OPEN_CONNECTION:	return "TP_REQ_OPEN_CONNECTION";
			case code::TP_RES_OPEN_CONNECTION:	return "TP_RES_OPEN_CONNECTION";

			//	transport - connection close
			case code::TP_REQ_CLOSE_CONNECTION:	return "TP_REQ_CLOSE_CONNECTION";
			case code::TP_RES_CLOSE_CONNECTION:	return "TP_RES_CLOSE_CONNECTION";

			//	transport - connection data transfer *** non-standard ***
			//	0x900B:	//	request
			//	0x100B:	//	response

			//	application
			//	application - protocol version
			case code::APP_REQ_PROTOCOL_VERSION:	return "APP_REQ_PROTOCOL_VERSION";
			case code::APP_RES_PROTOCOL_VERSION:	return "APP_RES_PROTOCOL_VERSION";

			//	application - device firmware version
			case code::APP_REQ_SOFTWARE_VERSION:	return "APP_REQ_SOFTWARE_VERSION";
			case code::APP_RES_SOFTWARE_VERSION:	return "APP_RES_SOFTWARE_VERSION";

			//	application - device identifier
			case code::APP_REQ_DEVICE_IDENTIFIER:	return "APP_REQ_DEVICE_IDENTIFIER";
			case code::APP_RES_DEVICE_IDENTIFIER:	return "APP_RES_DEVICE_IDENTIFIER";

			//	application - network status
			case code::APP_REQ_NETWORK_STATUS:	return "APP_REQ_NETWORK_STATUS";
			case code::APP_RES_NETWORK_STATUS:	return "APP_RES_NETWORK_STATUS";

			//	application - IP statistic
			case code::APP_REQ_IP_STATISTICS:	return "APP_REQ_IP_STATISTICS";
			case code::APP_RES_IP_STATISTICS:	return "APP_RES_IP_STATISTICS";

			//	application - device authentification
			case code::APP_REQ_DEVICE_AUTHENTIFICATION:	return "APP_REQ_DEVICE_AUTHENTIFICATION";
			case code::APP_RES_DEVICE_AUTHENTIFICATION:	return "APP_RES_DEVICE_AUTHENTIFICATION";

			//	application - device time
			case code::APP_REQ_DEVICE_TIME:	return "APP_REQ_DEVICE_TIME";
			case code::APP_RES_DEVICE_TIME:	return "APP_RES_DEVICE_TIME";

			//	application - push-target namelist
			case code::APP_REQ_PUSH_TARGET_NAMELIST:	return "APP_REQ_PUSH_TARGET_NAMELIST";
			case code::APP_RES_PUSH_TARGET_NAMELIST:	return "APP_RES_PUSH_TARGET_NAMELIST";

			//	application - push-target echo
			case code::APP_REQ_PUSH_TARGET_ECHO:	return "APP_REQ_PUSH_TARGET_ECHO";
			case code::APP_RES_PUSH_TARGET_ECHO:	return "APP_RES_PUSH_TARGET_ECHO";

			//	application - traceroute
			case code::APP_REQ_TRACEROUTE:	return "APP_REQ_TRACEROUTE";
			case code::APP_RES_TRACEROUTE:	return "APP_RES_TRACEROUTE";

			case code::CTRL_RES_LOGIN_PUBLIC:	return "CTRL_RES_LOGIN_PUBLIC";
			case code::CTRL_RES_LOGIN_SCRAMBLED:	return "CTRL_RES_LOGIN_SCRAMBLED";

			//	control
			case code::CTRL_REQ_LOGIN_PUBLIC:	return "CTRL_REQ_LOGIN_PUBLIC";
			case code::CTRL_REQ_LOGIN_SCRAMBLED:	return "CTRL_REQ_LOGIN_SCRAMBLED";

			//	control - maintenance
			case code::MAINTENANCE_REQUEST:	return "MAINTENANCE_REQUEST";
			case code::MAINTENANCE_RESPONSE:	return "MAINTENANCE_RESPONSE";

			//	control - logout
			case code::CTRL_REQ_LOGOUT:	return "CTRL_REQ_LOGOUT";
			case code::CTRL_RES_LOGOUT:	return "CTRL_RES_LOGOUT";

			//	control - push target register
			case code::CTRL_REQ_REGISTER_TARGET:	return "CTRL_REQ_REGISTER_TARGET";
			case code::CTRL_RES_REGISTER_TARGET:	return "CTRL_RES_REGISTER_TARGET";

			//	control - push target deregister
			case code::CTRL_REQ_DEREGISTER_TARGET:	return "CTRL_REQ_DEREGISTER_TARGET";
			case code::CTRL_RES_DEREGISTER_TARGET:	return "CTRL_RES_DEREGISTER_TARGET";

			//	control - watchdog
			case code::CTRL_REQ_WATCHDOG:	return "CTRL_REQ_WATCHDOG";
			case code::CTRL_RES_WATCHDOG:	return "CTRL_RES_WATCHDOG";

			case code::UNKNOWN:	return "UNKNOWN";

			default:
				BOOST_ASSERT_MSG(false, "undefined IP-T command");
				break;
			}

			return "undefined";
		}
	}	//	ipt	
}
