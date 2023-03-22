#include <smf/dns/op_code.h>

namespace smf {
    namespace dns {
        const char *op_code_name(std::uint8_t c) {
            switch (to_op_code(c)) {

            case op_code::QUERY: return "QUERY";
            case op_code::INVERSE: return "INVERSE";
            case op_code::STATUS: return "STATUS";
            case op_code::RESERVED_03: return "RESERVED(3)";
            case op_code::NOTIFY: return "NOTIFY";
            case op_code::UPDATE: return "UPDATE";
            case op_code::RESERVED_06: return "RESERVED(6)";
            case op_code::RESERVED_07: return "RESERVED(7)";
            case op_code::RESERVED_08: return "RESERVED(8)";
            case op_code::RESERVED_09: return "RESERVED(9)";
            case op_code::RESERVED_10: return "RESERVED(10)";
            case op_code::RESERVED_11: return "RESERVED(11)";
            case op_code::RESERVED_12: return "RESERVED(12)";
            case op_code::RESERVED_13: return "RESERVED(13)";
            case op_code::RESERVED_14: return "RESERVED(14)";
            case op_code::RESERVED_15: return "RESERVED(15)";
            default: break;
            }
            return "undefined";
        }
    } // namespace dns
} // namespace smf
