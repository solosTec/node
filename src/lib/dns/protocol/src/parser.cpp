#include <smf/dns/parser.h>

#include <boost/assert.hpp>

namespace smf {
    namespace dns {
        parser::parser() {}

        auto parse_name(const uint8_t *p) noexcept -> std::string {
            if (p[0] == 0xC0) { // compression name
                return std::string(p, p + 2);
            }
            if (p[0] == 0x00) { // root name
                return std::string(p, p + 1);
            }
            size_t size = std::strlen(reinterpret_cast<const char *const>(p) + 1);
            return std::string(p, p + size + 2);
        }
    } // namespace dns
} // namespace smf
