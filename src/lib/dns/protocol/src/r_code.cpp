#include <smf/dns/r_code.h>

namespace smf {
    namespace dns {
        const char *r_code_name(std::uint8_t c) {
            switch (to_r_code(c)) {

            case r_code::SUCCESS: return "NOERROR";
            case r_code::FORMERR: return "FORMERR";
            case r_code::SERVFAIL: return "SERVFAIL";
            case r_code::NXDOMAIN: return "NXDOMAIN";
            case r_code::NOTIMP: return "NOTIMP";
            case r_code::REFUSED: return "REFUSED";
            case r_code::YXDOMAIN: return "YXDOMAIN";
            case r_code::YXRRSET: return "YXRRSET";
            case r_code::NXRRSET: return "NXRRSET";
            case r_code::NOTAUTH: return "NOTAUTH";
            case r_code::NOTZONE: return "NOTZONE";
            case r_code::RESERVED_11: return "RESERVED(11)";
            case r_code::RESERVED_12: return "RESERVED(12)";
            case r_code::RESERVED_13: return "RESERVED(13)";
            case r_code::RESERVED_14: return "RESERVED(14)";
            case r_code::RESERVED_15: return "RESERVED(15)";
            case r_code::BADVERS: return "BADVERS";
            default: break;
            }
            return "undefined";
        }
    } // namespace dns
} // namespace smf
