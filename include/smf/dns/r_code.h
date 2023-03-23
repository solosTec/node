/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_DNS_R_H
#define SMF_DNS_R_H

#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace smf {
    namespace dns {
        /**
         * The op code is encoded in 4 bits, so 16 different values are possible.
         */
        enum class r_code : std::uint8_t {
            //  NOERROR is defined in winerror.h
            SUCCESS = 0, //!< 0: No error (RFC1035)
            FORMERR,     //!< 1: Format error (RFC1035)
            SERVFAIL,    //!< 2: Server failure (RFC1035)
            NXDOMAIN,    //!< 3: Name Error (RFC1035)
            NOTIMP,      //!< 4: Not Implemented (RFC1035)
            REFUSED,     //!< 5: Refused (RFC1035)
            YXDOMAIN,    //!< 6: Name unexpectedly exists (RFC2136)
            YXRRSET,     //!< 7: RRset unexpectedly exists (RFC2136)
            NXRRSET,     //!< 8: RRset should exist but not (RFC2136)
            NOTAUTH,     //!< 9: Server isn't authoritative (RFC2136)
            NOTZONE,     //!< 10: Name is not within the zone (RFC2136)
            RESERVED_11, //!< 11: Reserved for future use (RFC1035)
            RESERVED_12, //!< 12: Reserved for future use (RFC1035)
            RESERVED_13, //!< 13: Reserved for future use (RFC1035)
            RESERVED_14, //!< 14: Reserved for future use (RFC1035)
            RESERVED_15, //!< 15: Reserved for future use (RFC1035)
            BADVERS,     //!< 16: EDNS version not implemented (RFC2671)
            INVALID
        };

        /**
         *	@return name of the r code
         */
        const char *r_code_name(std::uint8_t);

        constexpr r_code to_r_code(std::uint8_t q) { return static_cast<r_code>(q); }

    } // namespace dns

    template <typename ch, typename char_traits>
    std::basic_ostream<ch, char_traits> &operator<<(std::basic_ostream<ch, char_traits> &os, dns::r_code op) {
        os << dns::r_code_name(static_cast<std::uint8_t>(op));
        return os;
    }

} // namespace smf

#endif
