/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_DNS_OP_H
#define SMF_DNS_OP_H

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
        enum class op_code : std::uint8_t {
            QUERY = 0,   //!< 0: Standard query (RFC1035)
            INVERSE,     //!< 1: Inverse query (RFC1035)
            STATUS,      //!< 2: Server status request (RFC1035)
            RESERVED_03, //!< 3: Reserved for future use (RFC1035)
            NOTIFY,      //!< 4: Notify (RFC1996)
            UPDATE,      //!< 5: Dynamic update (RFC2136)
            RESERVED_06, //!< 6: Reserved for future use (RFC1035)
            RESERVED_07, //!< 7: Reserved for future use (RFC1035)
            RESERVED_08, //!< 8: Reserved for future use (RFC1035)
            RESERVED_09, //!< 9: Reserved for future use (RFC1035)
            RESERVED_10, //!< 10: Reserved for future use (RFC1035)
            RESERVED_11, //!< 11: Reserved for future use (RFC1035)
            RESERVED_12, //!< 12: Reserved for future use (RFC1035)
            RESERVED_13, //!< 13: Reserved for future use (RFC1035)
            RESERVED_14, //!< 14: Reserved for future use (RFC1035)
            RESERVED_15, //!< 15: Reserved for future use (RFC1035)
            INVALID
        };

        /**
         *	@return name of the IP-T command
         */
        const char *op_code_name(std::uint8_t);

        constexpr op_code to_op_code(std::uint8_t q) { return static_cast<op_code>(q); }

    } // namespace dns

    template <typename ch, typename char_traits>
    std::basic_ostream<ch, char_traits> &operator<<(std::basic_ostream<ch, char_traits> &os, dns::op_code op) {
        os << dns::op_code_name(static_cast<std::uint8_t>(op));
        return os;
    }

} // namespace smf

#endif
