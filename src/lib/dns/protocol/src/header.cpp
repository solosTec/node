#include <smf/dns/header.h>

namespace smf {
    namespace dns {
        msg::msg()
            : data_{0} {}

        std::uint16_t msg::get_id() const noexcept {
            // net
            return data_[1] + (data_[0] << 8);
        }

        bool msg::is_reponse_code() const noexcept {
            //  test last bit byte #2
            constexpr std::uint8_t mask{0b0000'0001}; // represents bit 0
            return (data_[2] & mask) == mask;
        }

        op_code msg::get_opcode() const noexcept {
            constexpr std::uint8_t mask{0b0001'1110}; // represents bit 1-4
            std::uint8_t code = (data_[2] & mask) >> 1;
            return (code < 0xF) ? static_cast<op_code>(code) : op_code::INVALID;
        }

    } // namespace dns
} // namespace smf
