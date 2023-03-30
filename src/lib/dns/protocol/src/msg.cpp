#include <smf/dns/msg.h>

namespace smf {
    namespace dns {

        header::header(data_type &data)
            : data_(data) {}

        std::uint16_t header::get_id() const noexcept {
            // net
            return data_[1] + (data_[0] << 8);
        }

        bool header::is_query() const noexcept {
            //  test last bit byte #2
            constexpr std::uint8_t mask{0b1000'0000}; // represents bit 0
            return !((data_[2] & mask) == mask);
        }

        bool header::is_recursion_desired() const noexcept {
            //  test last bit byte #2
            constexpr std::uint8_t mask{0b0000'0001};
            return (data_[2] & mask) == mask;
        }

        bool header::is_truncated() const noexcept {
            //  test byte #2
            constexpr std::uint8_t mask{0b0000'0010};
            return (data_[2] & mask) == mask;
        }

        op_code header::get_opcode() const noexcept {
            constexpr std::uint8_t mask{0b0111'1000}; // represents bit 1-4
            std::uint8_t code = (data_[2] & mask) >> 3;
            return (code < 0xF) ? static_cast<op_code>(code) : op_code::INVALID;
        }

        r_code header::get_rcode() const noexcept {
            constexpr std::uint8_t mask{0b0000'1111}; // represents bit 0-3
            std::uint8_t code = (data_[3] & mask);
            return (code < 0xF) ? static_cast<r_code>(code) : r_code::INVALID;
        }

        std::uint16_t header::get_qd_count() const noexcept { return data_[5] + (data_[4] << 8); }
        std::uint16_t header::get_an_count() const noexcept { return data_[7] + (data_[6] << 8); }
        std::uint16_t header::get_ns_count() const noexcept { return data_[9] + (data_[8] << 8); }
        std::uint16_t header::get_ar_count() const noexcept { return data_[11] + (data_[10] << 8); }

        msg::msg()
            : data_{0}
            , header_(data_)
            , qname_{} {}

        header const &msg::get_header() const { return header_; }

    } // namespace dns
} // namespace smf
