#include <smf/sml.h>

#ifdef _DEBUG_SML
#include <iostream>
#endif

#include <map>

#include <boost/algorithm/string.hpp>
#include <boost/assert.hpp>

namespace smf {
    namespace sml {

        const char *get_name(sml_type type) {
            switch (type) {
            case sml_type::BINARY:
                return "BINARY";
            case sml_type::BOOLEAN:
                return "BOOLEAN";
            case sml_type::INTEGER:
                return "INTEGER";
            case sml_type::UNSIGNED:
                return "UNSIGNED";
            case sml_type::LIST:
                return "LIST";
            case sml_type::OPTIONAL:
                return "OPTIONAL";
            case sml_type::EOM:
                return "EOM";
            case sml_type::RESERVED:
                return "RESERVED";
            default:
                break;
            }
            return "unknown";
        }

        /**
         * map all message type enums to strings
         */
        static std::map<msg_type, std::string> msg_types{
            {msg_type::OPEN_REQUEST, "OPEN_REQUEST"},
            {msg_type::OPEN_RESPONSE, "OPEN_RESPONSE"},
            {msg_type::CLOSE_REQUEST, "CLOSE_REQUEST"},
            {msg_type::CLOSE_RESPONSE, "CLOSE_RESPONSE"},
            {msg_type::GET_PROFILE_PACK_REQUEST, "GET_PROFILE_PACK_REQUEST"},
            {msg_type::GET_PROFILE_PACK_RESPONSE, "GET_PROFILE_PACK_RESPONSE"},
            {msg_type::GET_PROFILE_LIST_REQUEST, "GET_PROFILE_LIST_REQUEST"},
            {msg_type::GET_PROFILE_LIST_RESPONSE, "GET_PROFILE_LIST_RESPONSE"},
            {msg_type::GET_PROC_PARAMETER_REQUEST, "GET_PROC_PARAMETER_REQUEST"},
            {msg_type::GET_PROC_PARAMETER_RESPONSE, "GET_PROC_PARAMETER_RESPONSE"},
            {msg_type::SET_PROC_PARAMETER_REQUEST, "SET_PROC_PARAMETER_REQUEST"},
            {msg_type::SET_PROC_PARAMETER_RESPONSE, "SET_PROC_PARAMETER_RESPONSE"},
            {msg_type::GET_LIST_REQUEST, "GET_LIST_REQUEST"},
            {msg_type::GET_LIST_RESPONSE, "GET_LIST_RESPONSE"},

            //	since v1.04
            {msg_type::GET_COSEM_REQUEST, "GET_COSEM_REQUEST"},
            {msg_type::GET_COSEM_RESPONSE, "GET_COSEM_RESPONSE"},
            {msg_type::SET_COSEM_REQUEST, "SET_COSEM_REQUEST"},
            {msg_type::SET_COSEM_RESPONSE, "SET_COSEM_RESPONSE"},
            {msg_type::ACTION_COSEM_REQUEST, "ACTION_COSEM_REQUEST"},
            {msg_type::ACTION_COSEM_RESPONSE, "ACTION_COSEM_RESPONSE"},

            {msg_type::ATTENTION_RESPONSE, "ATTENTION_RESPONSE"},
        };

        std::string get_name(msg_type type) {

            auto const pos = msg_types.find(type);
            return (pos != msg_types.end()) ? pos->second : "UNKNOWN";
        }

        msg_type msg_type_by_name(std::string name) {
            for (auto const &e : msg_types) {
                if (boost::algorithm::equals(e.second, name))
                    return e.first;
            }
            return msg_type::UNKNOWN;
        }
    } // namespace sml
} // namespace smf
