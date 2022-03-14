#include <smf/obis/defs.h>
#include <smf/sml/attention.h>

namespace smf {
    namespace sml {
        cyng::obis get_code(attention_type type) {
            switch (type) {
            case attention_type::OK:
                return OBIS_ATTENTION_OK;
            case attention_type::JOB_IS_RUNNINNG:
                return OBIS_ATTENTION_JOB_IS_RUNNINNG;
            case attention_type::UNKNOWN_ERROR:
                return OBIS_ATTENTION_UNKNOWN_ERROR;
            case attention_type::UNKNOWN_SML_ID:
                return OBIS_ATTENTION_UNKNOWN_SML_ID;
            case attention_type::NOT_AUTHORIZED:
                return OBIS_ATTENTION_NOT_AUTHORIZED;
            case attention_type::NO_SERVER_ID:
                return OBIS_ATTENTION_NO_SERVER_ID;
            case attention_type::NO_REQ_FIELD:
                return OBIS_ATTENTION_NO_REQ_FIELD;
            case attention_type::CANNOT_WRITE:
                return OBIS_ATTENTION_CANNOT_WRITE;
            case attention_type::CANNOT_READ:
                return OBIS_ATTENTION_CANNOT_READ;
            case attention_type::COMM_ERROR:
                return OBIS_ATTENTION_COMM_ERROR;
            case attention_type::PARSER_ERROR:
                return OBIS_ATTENTION_PARSER_ERROR;
            case attention_type::OUT_OF_RANGE:
                return OBIS_ATTENTION_OUT_OF_RANGE;
            case attention_type::NOT_EXECUTED:
                return OBIS_ATTENTION_NOT_EXECUTED;
            case attention_type::INVALID_CRC:
                return OBIS_ATTENTION_INVALID_CRC;
            case attention_type::NO_BROADCAST:
                return OBIS_ATTENTION_NO_BROADCAST;
            case attention_type::UNEXPECTED_MSG:
                return OBIS_ATTENTION_UNEXPECTED_MSG;
            case attention_type::UNKNOWN_OBIS_CODE:
                return OBIS_ATTENTION_UNKNOWN_OBIS_CODE;
            case attention_type::UNSUPPORTED_DATA_TYPE:
                return OBIS_ATTENTION_UNSUPPORTED_DATA_TYPE;
            case attention_type::ELEMENT_NOT_OPTIONAL:
                return OBIS_ATTENTION_ELEMENT_NOT_OPTIONAL;
            case attention_type::NO_ENTRIES:
                return OBIS_ATTENTION_NO_ENTRIES;
            case attention_type::END_LIMIT_BEFORE_START:
                return OBIS_ATTENTION_END_LIMIT_BEFORE_START;
            case attention_type::NO_ENTRIES_IN_RANGE:
                return OBIS_ATTENTION_NO_ENTRIES_IN_RANGE;
            case attention_type::MISSING_CLOSE_MSG:
                return OBIS_ATTENTION_MISSING_CLOSE_MSG;
            default:
                break;
            }
            return OBIS_ATTENTION_UNKNOWN_ERROR;
        }
    } // namespace sml
} // namespace smf
