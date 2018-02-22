/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_DEFS_H
#define NODE_SML_DEFS_H

/** @file defs.h
 * lib_protocol_sml is a library which implements the Smart Message Language (SML) protocol specified 
 * by VDE's Forum Netztechnik/Netzbetrieb (FNN). It can be utilized to communicate to FNN specified 
 * Smart Meters or Smart Meter components (EDL/MUC).
 */

#include <cyng/intrinsics/buffer.h>
#include <cstdint>
#include <type_traits>
#include <exception>
#include <string>

namespace node
{
	namespace sml	
	{

		//	special values
		enum { ESCAPE_SIGN = 27 };


		enum sml_types_enum
		{
			SML_STRING = 0,
			SML_BOOLEAN = 4,
			SML_INTEGER = 5,
			SML_UNSIGNED = 6,
			SML_LIST = 7,	//	sequence
			SML_OPTIONAL,
			SML_EOM,		//	end of SML message
			SML_RESERVED,
			SML_UNKNOWN,
		};

		enum sml_messages_enum : std::uint16_t
		{
			BODY_OPEN_REQUEST = 0x00000100,
			BODY_OPEN_RESPONSE = 0x00000101,
			BODY_CLOSE_REQUEST = 0x0000200,
			BODY_CLOSE_RESPONSE = 0x00000201,
			BODY_GET_PROFILE_PACK_REQUEST = 0x00000300,
			BODY_GET_PROFILE_PACK_RESPONSE = 0x00000301,
			BODY_GET_PROFILE_LIST_REQUEST = 0x00000400,
			BODY_GET_PROFILE_LIST_RESPONSE = 0x00000401,
			BODY_GET_PROC_PARAMETER_REQUEST = 0x00000500,
			BODY_GET_PROC_PARAMETER_RESPONSE = 0x00000501,
			BODY_SET_PROC_PARAMETER_REQUEST = 0x00000600,
			BODY_SET_PROC_PARAMETER_RESPONSE = 0x00000601,
			BODY_GET_LIST_REQUEST = 0x00000700,
			BODY_GET_LIST_RESPONSE = 0x00000701,
			BODY_ATTENTION_RESPONSE = 0x0000FF01,
			BODY_UNKNOWN,
		};

		namespace messages
		{
			inline const char* name(std::uint16_t code)
			{
				switch (code)
				{
				case BODY_OPEN_REQUEST:	return "PublicOpenRequest";
				case BODY_OPEN_RESPONSE:	return "PublicOpenResponse";
				case BODY_CLOSE_REQUEST:	return "PublicCloseRequest";
				case BODY_CLOSE_RESPONSE:	return "PublicCloseResponse";
				case BODY_GET_PROFILE_PACK_REQUEST:	return "GetPrpfilePackRequest";
				case BODY_GET_PROFILE_PACK_RESPONSE:	return "GetProfilePackResponse";
				case BODY_GET_PROFILE_LIST_REQUEST:	return "GetProfileListRequest";
				case BODY_GET_PROFILE_LIST_RESPONSE:	return "GetProfileListResponse";
				case BODY_GET_PROC_PARAMETER_REQUEST:	return "GetProcParameterRequest";
				case BODY_GET_PROC_PARAMETER_RESPONSE:	return "GetProcParameterResponse";
				case BODY_SET_PROC_PARAMETER_REQUEST:	return "SetProcParameterRequest";
				case BODY_SET_PROC_PARAMETER_RESPONSE:	return "SetProcParameterResponse";
				case BODY_GET_LIST_REQUEST:	return "GetListRequest";
				case BODY_GET_LIST_RESPONSE:	return "GetListResponse";
				case BODY_ATTENTION_RESPONSE:	return "Attention";
				default:
					break;
				}

				return "";
			}
		}

		enum sml_token_enum
		{
			TOKEN_MESSAGE,
			TOKEN_BODY,
			TOKEN_TREE_PATH,
			TOKEN_TIME,
			TOKEN_VALUE,
			TOKEN_VALUE_ENTRY,
		};

		enum sml_proc_par_value : std::uint8_t
		{
			PROC_PAR_VALUE = 1,
			PROC_PAR_PERIODENTRY = 2,
			PROC_PAR_TUPELENTRY = 3,
			PROC_PAR_TIME = 4,
		};

		/**
		 *	time value types
		 */
		enum sml_time_enum
		{
			TIME_SECINDEX = 1,
			TIME_TIMESTAMP = 2,
		};

		struct sml_tl
		{
			std::size_t	length_;
			sml_types_enum	type_;
		};

		//	typedef for octet string
		using octet_type = cyng::buffer_t;


		using SML_ESCAPE	= std::integral_constant<std::uint32_t, 0x1b1b1b1b>;
		using SML_VERSION_1 = std::integral_constant<std::uint32_t, 0x01010101>;

		enum address_type : std::uint8_t	
		{
			MBUS_WIRELESS = 0x01,	//!<	(1) Die 8 Bytes der Wireless-M-Bus-Adresse werden direkt im Anschluss an das Byte 0 (MSB)angeordnet
			MBUS_WIRED = 0x02,		//!<	(2) Die 8 Bytes der Wired-M-Bus-Adresse werden direkt im Anschluss an das Byte 0 (MSB)angeordnet
			RHEIN_ENERGY = 0x03,	//!<	(3) 9 bytes
			E_ON = 0x04,			//!<	(4) 7 bytes
			MAC_ADDRESS = 0x05,		//!<	(5) 6 bytes (MUC, gateway)
			DKE_1 = 0x06,			//!<	(6) 6 bytes (E DIN 43863-5:2010-02)
			IMEI = 0x07,			//!<	(7) 7 bytes
			RWE = 0x08,				//!<	(8) 7 bytes
			DKE_2 = 0x09,			//!<	(9) 10 bytes (E DIN 43863-5:2010-07)
			ACTOR = 0x0A,			//!<	(0x0A) 
			SUB_ADDRESS = 0x7F,		//!<	(0x7F) 

			//	special cases
//			SERIAL_NUMBER,			//!<	8 bytes as ASCII code: example "30 32 34 36 37 34 38 36" this is the serial number of the M-Bus encoding

			RESERVED,
			OTHER
		};


	}	//	sml
}	//	node


#endif	//	NODE_SML_DEFS_H
