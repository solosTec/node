/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_FIELD_DEFINITIONS_H
#define SMF_MBUS_FIELD_DEFINITIONS_H

#include <string>
#include <cstdint>

namespace smf
{
	namespace mbus
	{

		//
		//	max frame size is 252 bytes
		//
		constexpr std::uint8_t FRAME_DATA_LENGTH = 0xFC;

		//
		// Frame start/stop bits
		//
		constexpr std::uint8_t FRAME_ACK_START = 0xE5;	//	acknowledge receipt of transmissions
		constexpr std::uint8_t FRAME_SHORT_START = 0x10;
		constexpr std::uint8_t FRAME_CONTROL_START = 0x68;
		constexpr std::uint8_t FRAME_LONG_START = 0x68;
		constexpr std::uint8_t FRAME_STOP = 0x16;

		//
		//	C-Fields
		//	see https://oms-group.org/fileadmin/files/download4all/specification/Vol2/4.1.2/OMS-Spec_Vol2_Primary_v412.pdf
		//
		//constexpr std::uint8_t CTRL_FIELD_SP_UD = 0x08;	//	
		//constexpr std::uint8_t CTRL_FIELD_IR = 0x00;		//	Send manually initiated installation data � Send installation request
		//constexpr std::uint8_t CTRL_FIELD_ACC_NR = 0x00;	//	No data � Provides opportunity to access the meter between two application transmissions
		//constexpr std::uint8_t CTRL_FIELD_ACC_DMD = 0x00;	//	Access demand to master in order to request new importat application data � Alerts
		//constexpr std::uint8_t CTRL_FIELD_ACC = 0x00;		//	Acknowledge the reception 
		//constexpr std::uint8_t CTRL_FIELD_RSP_UD = 0x00;	//	Response of application data after a request from master

		enum c_field_master : std::uint8_t {
			CTRL_FIELD_SND_NKE = 0x40,	//	Link reset after communication - SND_NKE

			CTRL_FIELD_SND_UD2 = 0x43,	//	Send command with subsequent response - RSP_UD
			CTRL_FIELD_SND_UD_CMD1 = 0x53,	//	Send command (send user data to slave) - ACK
			CTRL_FIELD_SND_UD_CMD2 = 0x73,	//	Send command (send user data to slave) - ACK

			CTRL_FIELD_REQ_UD1_ACK = 0x5A,	//	Request User Data
			CTRL_FIELD_REQ_UD1_RSP_UD = 0x7A,	//	Request User Data

			CTRL_FIELD_REQ_UD2_CLASS1 = 0x5B,	//	Request User Data
			CTRL_FIELD_REQ_UD2_CLASS2 = 0x7B,	//	Request User Data

			CTRL_FIELD_ACK = 0x00,	//	Acknowledge the reception of the ACC-DMD
			CTRL_FIELD_CNF_IR = 0x06,	//	Confirms the successful registration (installation) ofmeter/actuator into this gateway
		};

		enum c_field_slave : std::uint8_t {
			CTRL_FIELD_SND_NR = 0x44,	//	Send spontaneous/periodical application data without request (S1 mode)
			CTRL_FIELD_SND_IR = 0x46,	//	Send manually initiated installation data (Send Installation Request)
			CTRL_FIELD_ACC_NR = 0x47,	//	Contains no data � signals an empty transmission or provides the opportunity to access the bidirectional meter, between two application data transmissions
			CTRL_FIELD_ACC_DMD = 0x48,	//	Access demand to master in order to request new important application data (alerts)

			//	Acknowledge the reception of a SND-UD (acknowledgement of transmission only); It shall also be used as a response to an REQ-UD1, when no alert happened
			//CTRL_FIELD_ACK = 0x00, 
			CTRL_FIELD_ACK_1 = 0x10,
			CTRL_FIELD_ACK_2 = 0x20,
			CTRL_FIELD_ACK_3 = 0x30,

			//	Response of application data after a request from master(response of user data)
			CTRL_FIELD_RSP_UD = 0x08,
			CTRL_FIELD_RSP_UD_1 = 0x18,
			CTRL_FIELD_RSP_UD_2 = 0x28,
			CTRL_FIELD_RSP_UD_3 = 0x38,

		};

		//
		//	CONTROL INFORMATION FIELD (CI-field)
		//

		enum ci_field : std::uint8_t {

			FIELD_CI_RESET = 0x50, //!<		Reset(EN 13757-3)
			FIELD_CI_DATA_SEND = 0x51, //!<	Data send - mode 1 (EN 13757-3)
			FIELD_CI_SLAVE_SELECT = 0x52,	//!<	Slave select - mode 1 (EN 13757-3)
			FIELD_CI_APPLICATION_RESET = 0x53,	//!<	Application reset or select (EN 13757-3).
			FIELD_CI_SYNC = 0x54,	//!< syncronize action
			FIELD_CI_CMD_TO_DEVICE_SHORT = 0x5A,	//!<	CMD to device with short header (OMS Vol.2 Issue 2.0.0/2009-07-20)								
			FIELD_CI_CMD_TO_DEVICE_LONG = 0x5B,	//!<	CMD to device with long header (OMS Vol.2 Issue 2.0.0/2009-07-20)
			FIELD_CI_CMD_TLS_HANDSHAKE = 0x5F,	//!<	Security Management (TLS-Handshake)
			FIELD_CI_CMD_DLMS_LONG = 0x60,
			FIELD_CI_CMD_DLMS_SHORT = 0x61,
			FIELD_CI_CMD_SML_LONG = 0x64,
			FIELD_CI_CMD_SML_SHORT = 0x65,
			FIELD_CI_TIME_SYNC_1 = 0x6C,	//!< Time synchronization (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
			FIELD_CI_TIME_SYNC_2 = 0x6D, //!<	Time synchronization (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
			FIELD_CI_APL_ERROR_SHORT = 0x6E, //!<	Error from device with short header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
			FIELD_CI_APL_ERROR_LONG = 0x6F, //!<	Error from device with long header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
			FIELD_CI_APL_ERROR = 0x70, //!<		Report of general application errors (EN 13757-3)
			FIELD_CI_ALARM = 0x71, //!<		report of alarms (EN 13757-3)
			FIELD_CI_HEADER_LONG = 0x72, //!<	12 byte header followed by variable format data (EN 13757-3)
			FIELD_CI_APL_ALARM_SHORT = 0x74, //!	<Alarm from device with short header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
			FIELD_CI_APL_ALARM_LONG = 0x75, //!<	Alarm from device with long header (OMS Vol.2 Issue 3.0.0xxxx-xx-xx)
			FIELD_CI_HEADER_NO = 0x78, //!<	Variable data format respond without header (EN 13757-3)
			FIELD_CI_HEADER_SHORT = 0x7A, //!<	4 byte header followed by variable data format respond (EN 13757-3)
			FIELD_CI_RES_LONG_DLMS = 0x7C,
			FIELD_CI_RES_SHORT_DLSM = 0x7D,	//!<	short header
			FIELD_CI_RES_LONG_SML = 0x7E,
			FIELD_CI_RES_SHORT_SML = 0x7F,	//!<	short header
			FIELD_CI_LINK_TO_DEVICE_LONG = 0x80, //!<	Link extension to device with long header (OMS Vol.2 Issue 2.0.02009-07-20)
			FIELD_CI_LINK_FROM_DEVICE_SHORT = 0x8A, //!<	Link extension from device with short header
			FIELD_CI_LINK_FROM_DEVICE_LONG = 0x8B, //!<	Link extension from device with long header(OMS Vol.2 Issue 2.0.0/2009-07-20)

			//#if !OMS_V4_ENABLED

			FIELD_CI_EXT_DLL_I = 0x8C,		//!<	Additional Link Layer may be applied for Radio messages with or without Application Layer. 
			FIELD_CI_EXT_DLL_II = 0x8D, //!<	Additional Link Layer may be applied for Radio messages with or without Application Layer. 
			//#endif // OMS_V4_ENABLED

			FIELD_CI_EXT_DLL_III = 0x8E,	//!<	Lower Layer Service (10 Byte)

			FIELD_CI_AUTH_LAYER = 0x90,	//!<	Authentication and Fragmentation Layer (Lower Layer Service)

			FIELD_CI_RES_TLS_SHORT = 0x9E,	//!<	Security Management (TLS-Handshake)
			FIELD_CI_RES_TLS_LONG = 0x9F,	//!<	Security Management (TLS-Handshake)


			FIELD_CI_MANU_SPEC = 0xA0, //!<	Manufacture specific CI-Field
			FIELD_CI_MANU_NO = 0xA1, //!<	Manufacture specific CI-Field with no header
			FIELD_CI_MANU_SHORT = 0xA2, //!<	Manufacture specific CI-Field with short header
			FIELD_CI_MANU_LONG = 0xA3, //!<	Manufacture specific CI-Field with long header
			FIELD_CI_MANU_SHORT_RF = 0xA4, //!<	Manufacture specific CI-Field with short header (for RF-Test)

			FIELD_CU_BAUDRATE_300 = 0xB8,
			FIELD_CU_BAUDRATE_600 = 0xB9,
			FIELD_CU_BAUDRATE_1200 = 0xBA,
			FIELD_CU_BAUDRATE_2400 = 0xBB,
			FIELD_CU_BAUDRATE_4800 = 0xBC,
			FIELD_CU_BAUDRATE_9600 = 0xBD,
			FIELD_CU_BAUDRATE_19200 = 0xBE,
			FIELD_CU_BAUDRATE_38400 = 0xBF,

			FIELD_CI_NULL = 0xFF, //!<	No CI-field transmitted.
		};

		//
		//	81 06 19 07 04 FF - transmision power
		//
		enum transmission_power : std::uint8_t {
			BASIC = 0,	//	default
			LOW = 1,
			AVERAGE = 2,	//	medium
			STRONG = 3,		//	high
		};

		enum radio_protocol : std::uint8_t {

			MODE_S = 0,
			MODE_T = 1,
			MODE_C = 2,	//	alternating
			MODE_N = 3,	//	parallel
			MODE_S_SYNC = 4,	//	Used for synchronized messages in meters
			MODE_UNKNOWN = 5,
			RESERVED
		};


		enum encryption_mode : std::uint8_t {
			ENCRYPTION_MODE_0 = 0U,
			ENCRYPTION_MODE_5 = 5U,
			ENCRYPTION_MODE_7 = 7U,
			ENCRYPTION_MODE_9 = 9U,
			ENCRYPTION_MODE_15 = 15U,
			ENCRYPTION_MODE_UNKNOWN = 255U
		};

	}
}


#endif	
