/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_DEFS_H
#define NODE_IPT_DEFS_H

#include <string>
#include <cstdint>

namespace node
{
	/**
	 * List of parameters each device has to provide oprionally
	 */
	enum query_enum : std::uint32_t
	{
		//	application - protocol version (0xA000)
		QUERY_PROTOCOL_VERSION = (1 << 0),	// 0b0000000000000001

		//	application - device firmware version (0xA001)
		QUERY_FIRMWARE_VERSION = (1 << 1), // 0b0000000000000010

		//	application - device identifier (0xA003)
		QUERY_DEVICE_IDENTIFIER = (1 << 2),	// 0b0000000000000100

		//	application - network status (0xA004)
		QUERY_NETWORK_STATUS = (1 << 3),	// 0b0000000000001000

		//	application - IP statistic (0xA005)
		QUERY_IP_STATISTIC = (1 << 4),	//	0b0000000000010000

		//	application - device authentification (0xA006)
		QUERY_DEVICE_AUTHENTIFICATION = (1 << 5), // 0b0000000000100000

		//
		//	skip 6 
		//	skip 7
		//

		//	application - device time (0xA007)
		QUERY_DEVICE_TIME = (1 << 8),	// 0b0000000100000000

		QUERY_DEFAULT_VALUE = (QUERY_FIRMWARE_VERSION | QUERY_DEVICE_IDENTIFIER),
		QUERY_ALL_VALUES = (QUERY_PROTOCOL_VERSION
		| QUERY_FIRMWARE_VERSION
		| QUERY_DEVICE_IDENTIFIER
		| QUERY_NETWORK_STATUS
		| QUERY_IP_STATISTIC
		| QUERY_DEVICE_AUTHENTIFICATION
		| QUERY_DEVICE_TIME),
	};


	namespace ipt	
	{
		//	special values
		enum : std::uint8_t { ESCAPE_SIGN = 0x1b };	//!<	27dec
		enum { HEADER_SIZE = 8 };		//!<	header size in bytes

		//	define some standard sizes
		enum { NAME_LENGTH = 62 };
		enum { NUMBER_LENGTH = 79 };
//		enum { PASSWORD_LENGTH = 30 };	//	original limit was 30 bytes
		enum { PASSWORD_LENGTH = 36 };
		enum { ADDRESS_LENGTH = 255 };
		enum { TARGET_LENGTH = 255 };
		enum { DESCRIPTION_LENGTH = 255 };
		
		//	original limit was exeeded by many implementations i.g. "MUC-DSL-1.311_13220000__X022a_GWF3"
		//enum { VERSION_LENGTH		= 19 };
		enum { VERSION_LENGTH = 64 };

		//enum { DEVICE_ID_LENGTH		= 19 };
		enum { DEVICE_ID_LENGTH = 64 };	//	see VERSION_LENGTH
		enum { STATUS_LENGTH = 16 };

		typedef std::uint8_t		sequence_type;		//!<	sequence (1 - 255)
		typedef std::uint8_t		response_type;		//!<	more readable
		typedef std::uint16_t		command_type;		//!<	part of IPT header definition

		//	device_type GPRS (0x0) or LAN (0x1) slave
		typedef enum : std::uint8_t	
		{
			GPRS_DEVICE	= 0,
			LAN_DEVICE	= 1,
		} device_type_enum;

		/**
		 * value is interpreted in minutes.
		 * value 0 is no watchdog.
		 */
		constexpr std::uint16_t NO_WATCHDOG = 0;

	}	//	ipt
}	//	node


#endif	//	NODE_IPT_DEFS_H