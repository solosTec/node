#include <smf/dns/parser.h>

#include <cstring>

#include <boost/assert.hpp>

namespace smf {
    namespace dns {
        parser::parser()
            : state_(state::HEADER)
            , pos_{0}
            , msg_{} {}

        void parser::put(char c) {

            switch (state_) {
            case state::HEADER: state_ = state_header(c); break;
            case state::DATA: state_ = state_data(c); break;
            default: BOOST_ASSERT_MSG(false, "illegal parser state"); break;
            }
        }

        parser::state parser::state_header(char c) {
            BOOST_ASSERT(pos_ < msg::size());
            msg_.data_[pos_++] = c;
            if (pos_ == msg::size()) {
                pos_ = 0;
                return state::DATA;
            }
            return state_;
        }

        parser::state parser::state_data(char c) { return this->state_; }

#ifdef _DEBUG
        msg parser::get_msg() const { return msg_; }
#endif

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
