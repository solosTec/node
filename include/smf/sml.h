/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_H
#define SMF_SML_H

#include <cstdint>
#include <string>

#ifdef OPTIONAL
#undef OPTIONAL
#endif

namespace smf {
    namespace sml {

        //	special values
        constexpr std::uint8_t ESCAPE_SIGN = 27;

        enum class sml_type : std::uint8_t {
            BINARY = 0,
            BOOLEAN = 4,
            INTEGER = 5,
            UNSIGNED = 6,
            LIST = 7, //	sequence
            OPTIONAL,
            EOM, //	end of SML message
            RESERVED,
            UNKNOWN
        };

        /**
         * @return type name
         */
        const char *get_name(sml_type);

        enum class msg_type : std::uint16_t {
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
            GET_COSEM_REQUEST = 0x00000800,     //!<	SML_GETCOSEM.REQ
            GET_COSEM_RESPONSE = 0x00000801,    //!<	SML_GETCOSEM.RES
            SET_COSEM_REQUEST = 0x00000900,     //!<	SML_SETCOSEM.REQ
            SET_COSEM_RESPONSE = 0x00000901,    //!<	SML_SETCOSEM.RES
            ACTION_COSEM_REQUEST = 0x00000A00,  //!<	SML_ACTIONCOSEM.REQ
            ACTION_COSEM_RESPONSE = 0x00000A01, //!<	SML_ACTIONCOSEM.RES

            ATTENTION_RESPONSE = 0x0000FF01,
            UNKNOWN,
        };

        /**
         * get the enum value as u16
         */
        constexpr std::uint16_t to_u16(msg_type type) { return static_cast<std::uint16_t>(type); }

        constexpr msg_type to_msg_type(std::uint16_t type) {
            switch (type) {
            case to_u16(msg_type::OPEN_REQUEST):
                return msg_type::OPEN_REQUEST;
            case to_u16(msg_type::OPEN_RESPONSE):
                return msg_type::OPEN_RESPONSE;
            case to_u16(msg_type::CLOSE_REQUEST):
                return msg_type::CLOSE_REQUEST;
            case to_u16(msg_type::CLOSE_RESPONSE):
                return msg_type::CLOSE_RESPONSE;
            case to_u16(msg_type::GET_PROFILE_PACK_REQUEST):
                return msg_type::GET_PROFILE_PACK_REQUEST;
            case to_u16(msg_type::GET_PROFILE_PACK_RESPONSE):
                return msg_type::GET_PROFILE_PACK_RESPONSE;
            case to_u16(msg_type::GET_PROFILE_LIST_REQUEST):
                return msg_type::GET_PROFILE_LIST_REQUEST;
            case to_u16(msg_type::GET_PROFILE_LIST_RESPONSE):
                return msg_type::GET_PROFILE_LIST_RESPONSE;
            case to_u16(msg_type::GET_PROC_PARAMETER_REQUEST):
                return msg_type::GET_PROC_PARAMETER_REQUEST;
            case to_u16(msg_type::GET_PROC_PARAMETER_RESPONSE):
                return msg_type::GET_PROC_PARAMETER_RESPONSE;
            case to_u16(msg_type::SET_PROC_PARAMETER_REQUEST):
                return msg_type::SET_PROC_PARAMETER_REQUEST;
            case to_u16(msg_type::SET_PROC_PARAMETER_RESPONSE):
                return msg_type::SET_PROC_PARAMETER_RESPONSE;
            case to_u16(msg_type::GET_LIST_REQUEST):
                return msg_type::GET_LIST_REQUEST;
            case to_u16(msg_type::GET_LIST_RESPONSE):
                return msg_type::GET_LIST_RESPONSE;

                //	since v1.04
            case to_u16(msg_type::GET_COSEM_REQUEST):
                return msg_type::GET_COSEM_REQUEST;
            case to_u16(msg_type::GET_COSEM_RESPONSE):
                return msg_type::GET_COSEM_RESPONSE;
            case to_u16(msg_type::SET_COSEM_REQUEST):
                return msg_type::SET_COSEM_REQUEST;
            case to_u16(msg_type::SET_COSEM_RESPONSE):
                return msg_type::SET_COSEM_RESPONSE;
            case to_u16(msg_type::ACTION_COSEM_REQUEST):
                return msg_type::ACTION_COSEM_REQUEST;
            case to_u16(msg_type::ACTION_COSEM_RESPONSE):
                return msg_type::ACTION_COSEM_RESPONSE;

            case to_u16(msg_type::ATTENTION_RESPONSE):
                return msg_type::ATTENTION_RESPONSE;
            default:
                break;
            }

            return msg_type::UNKNOWN;
        }

        /**
         * @return type name
         */
        std::string get_name(msg_type);

        /**
         * @return type specified as string
         */
        msg_type msg_type_by_name(std::string name);

    } // namespace sml
} // namespace smf
#endif
