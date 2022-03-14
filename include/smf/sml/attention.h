/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_SML_ATTENTION
#define SMF_SML_ATTENTION

/** @file attention.h
 *
 * Attention codes are special OBIS codes that start with 81 81 C7 C7.
 * Each code describes the result of an "set process parameter" operation.
 */

#include <cyng/obj/intrinsics/obis.h>

namespace smf {
    namespace sml {
        /**
         * attention types
         */
        enum class attention_type : std::uint32_t {
            OK,                     // no error
            JOB_IS_RUNNINNG,        // job is running
            UNKNOWN_ERROR,          // unknown error
            UNKNOWN_SML_ID,         // unknown SML ID
            NOT_AUTHORIZED,         // not authorized
            NO_SERVER_ID,           // unable to find recipient for request
            NO_REQ_FIELD,           // no request field
            CANNOT_WRITE,           // cannot write
            CANNOT_READ,            // cannot read
            COMM_ERROR,             // communication error
            PARSER_ERROR,           // parser error
            OUT_OF_RANGE,           // out of range
            NOT_EXECUTED,           // not executed
            INVALID_CRC,            // invalid CRC
            NO_BROADCAST,           // no broadcast
            UNEXPECTED_MSG,         // unexpected message
            UNKNOWN_OBIS_CODE,      // unknown OBIS code
            UNSUPPORTED_DATA_TYPE,  // data type not supported
            ELEMENT_NOT_OPTIONAL,   // element is not optional
            NO_ENTRIES,             // no entries
            END_LIMIT_BEFORE_START, // end limit before start
            NO_ENTRIES_IN_RANGE,    // range is empty - not the profile
            MISSING_CLOSE_MSG,      // missing close message

        };

        /**
         * @return the OBIS attention code
         */
        cyng::obis get_code(attention_type);

    } // namespace sml
} // namespace smf

#endif
