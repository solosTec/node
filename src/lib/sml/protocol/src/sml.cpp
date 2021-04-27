#include <smf/sml.h>

#ifdef _DEBUG_SML
#include <iostream>
#endif

#include <boost/assert.hpp>

namespace smf {
	namespace sml {

		const char* get_name(sml_type type) {
			switch (type) {
			case sml_type::BINARY:	return "BINARY";
			case sml_type::BOOLEAN:	return "BOOLEAN";
			case sml_type::INTEGER:	return "INTEGER";
			case sml_type::UNSIGNED:	return "UNSIGNED";
			case sml_type::LIST:	return "LIST";
			case sml_type::OPTIONAL:	return "OPTIONAL";
			case sml_type::EOM:	return "EOM";
			case sml_type::RESERVED:	return "RESERVED";
			default:
				break;
			}
			return "unknown";
		}

		const char* get_name(msg_type type) {

			switch (type) {
			case msg_type::OPEN_REQUEST:	return "OPEN_REQUEST";
			case msg_type::OPEN_RESPONSE:	return "OPEN_RESPONSE";
			case msg_type::CLOSE_REQUEST:	return "CLOSE_REQUEST";
			case msg_type::CLOSE_RESPONSE:	return "CLOSE_RESPONSE";
			case msg_type::GET_PROFILE_PACK_REQUEST:	return "GET_PROFILE_PACK_REQUEST";
			case msg_type::GET_PROFILE_PACK_RESPONSE:	return "GET_PROFILE_PACK_RESPONSE";
			case msg_type::GET_PROFILE_LIST_REQUEST:	return "GET_PROFILE_LIST_REQUEST";
			case msg_type::GET_PROFILE_LIST_RESPONSE:	return "GET_PROFILE_LIST_RESPONSE";
			case msg_type::GET_PROC_PARAMETER_REQUEST:	return "GET_PROC_PARAMETER_REQUEST";
			case msg_type::GET_PROC_PARAMETER_RESPONSE:	return "GET_PROC_PARAMETER_RESPONSE";
			case msg_type::SET_PROC_PARAMETER_REQUEST:	return "SET_PROC_PARAMETER_REQUEST";
			case msg_type::SET_PROC_PARAMETER_RESPONSE:	return "SET_PROC_PARAMETER_RESPONSE";
			case msg_type::GET_LIST_REQUEST:	return "GET_LIST_REQUEST";
			case msg_type::GET_LIST_RESPONSE:	return "GET_LIST_RESPONSE";

			//	since v1.04
			case msg_type::GET_COSEM_REQUEST:		return "GET_COSEM_REQUEST";
			case msg_type::GET_COSEM_RESPONSE:		return "GET_COSEM_RESPONSE";
			case msg_type::SET_COSEM_REQUEST:			return "SET_COSEM_REQUEST";
			case msg_type::SET_COSEM_RESPONSE:	return "SET_COSEM_RESPONSE";
			case msg_type::ACTION_COSEM_REQUEST:	return "ACTION_COSEM_REQUEST";
			case msg_type::ACTION_COSEM_RESPONSE:	return "ACTION_COSEM_RESPONSE";

			case msg_type::ATTENTION_RESPONSE:	return "ATTENTION_RESPONSE";
			default:
				break;
			}

			return "UNKNOWN";
		}

	}
}
