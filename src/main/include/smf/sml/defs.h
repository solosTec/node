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
#include <boost/algorithm/string.hpp>

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

		enum class message_e : std::uint16_t
		{
			OPEN_REQUEST = 0x00000100,
			OPEN_RESPONSE = 0x00000101,
			CLOSE_REQUEST = 0x0000200,
			CLOSE_RESPONSE = 0x00000201,
			GET_PROFILE_PACK_REQUEST = 0x00000300,
			GET_PROFILE_PACK_RESPONSE = 0x00000301,
			GET_PROFILE_LIST_REQUEST = 0x00000400,
			GET_PROFILE_LIST_RESPONSE = 0x00000401,
			GET_PROC_PARAMETER_REQUEST = 0x00000500,
			GET_PROC_PARAMETER_RESPONSE = 0x00000501,
			SET_PROC_PARAMETER_REQUEST = 0x00000600,
			SET_PROC_PARAMETER_RESPONSE = 0x00000601,
			GET_LIST_REQUEST = 0x00000700,
			GET_LIST_RESPONSE = 0x00000701,

			//	since v1.04
			GET_COSEM_REQUEST = 0x00000800, //!<	SML_GETCOSEM.REQ
			GET_COSEM_RESPONSE = 0x00000801, //!<	SML_GETCOSEM.RES
			SET_COSEM_REQUEST = 0x00000900, //!<	SML_SETCOSEM.REQ
			SET_COSEM_RESPONSE = 0x00000901, //!<	SML_SETCOSEM.RES
			ACTION_COSEM_REQUEST = 0x00000A00, //!<	SML_ACTIONCOSEM.REQ
			ACTION_COSEM_RESPONSE = 0x00000A01, //!<	SML_ACTIONCOSEM.RES

			ATTENTION_RESPONSE = 0x0000FF01,
			UNKNOWN,
		};

		namespace messages
		{
			inline const char* name(message_e code) noexcept
			{
				switch (code)
				{
				case message_e::OPEN_REQUEST:	return "PublicOpenRequest";
				case message_e::OPEN_RESPONSE:	return "PublicOpenResponse";
				case message_e::CLOSE_REQUEST:	return "PublicCloseRequest";
				case message_e::CLOSE_RESPONSE:	return "PublicCloseResponse";
				case message_e::GET_PROFILE_PACK_REQUEST:	return "GetProfilePackRequest";
				case message_e::GET_PROFILE_PACK_RESPONSE:	return "GetProfilePackResponse";
				case message_e::GET_PROFILE_LIST_REQUEST:	return "GetProfileListRequest";
				case message_e::GET_PROFILE_LIST_RESPONSE:	return "GetProfileListResponse";
				case message_e::GET_PROC_PARAMETER_REQUEST:	return "GetProcParameterRequest";
				case message_e::GET_PROC_PARAMETER_RESPONSE:	return "GetProcParameterResponse";
				case message_e::SET_PROC_PARAMETER_REQUEST:	return "SetProcParameterRequest";
				case message_e::SET_PROC_PARAMETER_RESPONSE:	return "SetProcParameterResponse";
				case message_e::GET_LIST_REQUEST:	return "GetListRequest";
				case message_e::GET_LIST_RESPONSE:	return "GetListResponse";

				case message_e::GET_COSEM_REQUEST:	return "GetCOSEMRequest";
				case message_e::GET_COSEM_RESPONSE:	return "GetCOSEMResponse";
				case message_e::SET_COSEM_REQUEST:	return "SetCOSEMRequest";
				case message_e::SET_COSEM_RESPONSE:	return "SetCOSEMResponse";
				case message_e::ACTION_COSEM_REQUEST: return "ActionCOSEMRequest";
				case message_e::ACTION_COSEM_RESPONSE: return "ActionCOSEMResponse";

				case message_e::ATTENTION_RESPONSE:	return "Attention";
				default:
					break;
				}

				return "";
			}
			inline const char* name_from_value(std::uint16_t code)
			{
				return name(static_cast<message_e>(code));
			}

			inline message_e get_msg_code(std::string const& name) noexcept
			{
				//
				//	only requests
				//
				if (boost::algorithm::equals(name, "PublicOpenRequest")) return message_e::OPEN_REQUEST;
				else if (boost::algorithm::equals(name, "PublicOpenResponse")) return message_e::OPEN_RESPONSE;

				else if (boost::algorithm::equals(name, "PublicCloseRequest")) return message_e::CLOSE_REQUEST;
				else if (boost::algorithm::equals(name, "PublicCloseResponse")) return message_e::CLOSE_RESPONSE;

				else if (boost::algorithm::equals(name, "GetProfileListRequest"))	return message_e::GET_PROFILE_LIST_REQUEST;
				else if (boost::algorithm::equals(name, "GetProfileListResponse")) return message_e::GET_PROFILE_LIST_RESPONSE;

				else if (boost::algorithm::equals(name, "GetProfilePackRequest"))	return message_e::GET_PROFILE_PACK_REQUEST;
				else if (boost::algorithm::equals(name, "GetProfilePackResponse")) return message_e::GET_PROFILE_PACK_RESPONSE;

				else if (boost::algorithm::equals(name, "GetProcParameterRequest"))	return message_e::GET_PROC_PARAMETER_REQUEST;
				else if (boost::algorithm::equals(name, "GetProcParameterResponse")) return message_e::GET_PROC_PARAMETER_RESPONSE;

				else if (boost::algorithm::equals(name, "SetProcParameterRequest"))	return message_e::SET_PROC_PARAMETER_REQUEST;
				else if (boost::algorithm::equals(name, "SetProcParameterResponse")) return message_e::SET_PROC_PARAMETER_RESPONSE;

				else if (boost::algorithm::equals(name, "GetListRequest"))	return message_e::GET_LIST_REQUEST;
				else if (boost::algorithm::equals(name, "GetListResponse"))	return message_e::GET_LIST_RESPONSE;

				else if (boost::algorithm::equals(name, "GetCOSEMRequest"))		return message_e::GET_COSEM_REQUEST;
				else if (boost::algorithm::equals(name, "GetCOSEMResponse"))	return message_e::GET_COSEM_RESPONSE;
				else if (boost::algorithm::equals(name, "SetCOSEMRequest"))		return message_e::SET_COSEM_REQUEST;
				else if (boost::algorithm::equals(name, "SetCOSEMResponse"))	return message_e::SET_COSEM_RESPONSE;
				else if (boost::algorithm::equals(name, "ActionCOSEMRequest"))	return message_e::ACTION_COSEM_REQUEST;
				else if (boost::algorithm::equals(name, "ActionCOSEMResponse"))	return message_e::ACTION_COSEM_RESPONSE;

				else if (boost::algorithm::equals(name, "Attention"))	return message_e::ATTENTION_RESPONSE;

				return node::sml::message_e::UNKNOWN;
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
			PROC_PAR_UNDEF = 0,
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

		/**
		 * Access roles on gateway
		 */
		enum class role : std::uint8_t
		{
			GUEST = 1,
			USER,	//	end user
			GW_OPERATOR,
			DEV_OPERATOR,
			SERVICE_PROVIDER,
			SUPPLIER,
			MANUFACTURER,
			RESERVED
		};

		constexpr std::uint8_t get_bit_position(role r)
		{
			switch (r) {

			case role::GUEST:	return 1u;
			case role::USER:	return 2u;
			case role::GW_OPERATOR:	return 4u;
			case role::DEV_OPERATOR:	return 8u;
			case role::SERVICE_PROVIDER:	return 16u;
			case role::SUPPLIER:	return 32u;
			case role::MANUFACTURER:	return 64u;
			case role::RESERVED:	return 128u;
			default:
				break;
			}

			return 0u;	//	invalid value
		}

	}	//	sml
}	//	node


//
//  This is some kind of weird code and shouldn't be really be necessary.
//  But there are compiler error (gcc 7.5) /home/sol/projects/node/lib/sml/protocol/src/reader.cpp
//
//


#include <cyng/intrinsics/policy/hash.h>

namespace std
{
	template<> 
	struct hash<node::sml::message_e>
	{
		size_t operator()(node::sml::message_e e) const noexcept
		{
            return static_cast<std::uint16_t>(e);
        }
	};
}

#endif	//	NODE_SML_DEFS_H
