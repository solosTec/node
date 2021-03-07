/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_H
#define SMF_SML_H

#include <cstdint>

#ifdef OPTIONAL
#undef OPTIONAL
#endif


namespace smf {
	namespace sml {

		//	special values
		constexpr std::uint8_t ESCAPE_SIGN = 27;

		enum class sml_type : std::uint8_t
		{
			BINARY = 0,
			BOOLEAN = 4,
			INTEGER = 5,
			UNSIGNED = 6,
			LIST = 7,	//	sequence
			OPTIONAL,
			EOM,		//	end of SML message
			RESERVED,
			UNKNOWN
		};

		/**
		 * @return type name
		 */
		const char* get_name(sml_type);

		enum class msg_type : std::uint16_t
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

	}

}

//#include <smf/sml/parser.h>


#endif
