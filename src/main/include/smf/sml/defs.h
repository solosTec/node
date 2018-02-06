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
			inline std::string name(std::uint16_t code)
			{
				switch (code)
				{
				case BODY_OPEN_REQUEST:	return "PublicOpen_Req";
				case BODY_OPEN_RESPONSE:	return "PublicOpen_Res";
				case BODY_CLOSE_REQUEST:	return "PublicClose_Req";
				case BODY_CLOSE_RESPONSE:	return "PublicClose_Res";
				case BODY_GET_PROFILE_PACK_REQUEST:	return "BODY_GET_PROFILE_PACK_REQUEST";
				case BODY_GET_PROFILE_PACK_RESPONSE:	return "BODY_GET_PROFILE_PACK_RESPONSE";
				case BODY_GET_PROFILE_LIST_REQUEST:	return "BODY_GET_PROFILE_LIST_REQUEST";
				case BODY_GET_PROFILE_LIST_RESPONSE:	return "BODY_GET_PROFILE_LIST_RESPONSE";
				case BODY_GET_PROC_PARAMETER_REQUEST:	return "GetProcParameter_Req";
				case BODY_GET_PROC_PARAMETER_RESPONSE:	return "GetProcParameter_Res";
				case BODY_SET_PROC_PARAMETER_REQUEST:	return "BODY_SET_PROC_PARAMETER_REQUEST";
				case BODY_SET_PROC_PARAMETER_RESPONSE:	return "BODY_SET_PROC_PARAMETER_RESPONSE";
				case BODY_GET_LIST_REQUEST:	return "BODY_GET_LIST_REQUEST";
				case BODY_GET_LIST_RESPONSE:	return "BODY_GET_LIST_RESPONSE";
				case BODY_ATTENTION_RESPONSE:	return "BODY_ATTENTION_RESPONSE";
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

		/**
		 *	Defines a stack of obis codes describing the path from root to the value.
		 */
		//typedef std::vector<noddy::m2m::obis>	path_type;


	}	//	sml
}	//	node


#endif	//	NODE_SML_DEFS_H
