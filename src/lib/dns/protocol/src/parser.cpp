#include <smf/dns/parser.h>

#include <cstring>

#include <boost/assert.hpp>

namespace smf {
    namespace dns {
        parser::parser()
            : state_(state::HEADER)
            , pos_{0}
            , msg_{}
            , qname_(std::bind(&parser::name, this, std::placeholders::_1)) {}

        void parser::put(std::uint8_t c) {

            switch (state_) {
            case state::HEADER: state_ = state_header(c); break;
            case state::QNAME: state_ = state_qname(c); break;
            case state::QTYPE: state_ = state_qtype(c); break;
            case state::QCLASS: state_ = state_qclass(c); break;
            default: BOOST_ASSERT_MSG(false, "illegal parser state"); break;
            }
        }

        parser::state parser::state_header(std::uint8_t c) {
            BOOST_ASSERT(pos_ < header::size());
            msg_.data_[pos_++] = c;
            if (pos_ == header::size()) {
                pos_ = 0;
                qname_.reset();
                return state::QNAME;
            }
            return state_;
        }

        parser::state parser::state_qname(std::uint8_t c) {
            if (qname_.put(c) && (c == 0)) {
                //
                //  complete
                //
                return state::QTYPE;
            }
            return this->state_;
        }

        parser::state parser::state_qtype(std::uint8_t c) { return state_; }
        parser::state parser::state_qclass(std::uint8_t c) { return state_; }

#ifdef _DEBUG
        msg parser::get_msg() const { return msg_; }
#endif

        void parser::name(std::string name) {
#ifdef _DEBUG
            std::cout << "name: " << name << std::endl;
#endif
            if (msg_.get_header().is_query()) {
                //
                //  add to query
                //
                msg_.qname_.push_back(name);
            } else {
                //
                //  add to answer
                //
            }
        }

        parser::qname::qname(std::function<void(std::string)> cb_name)
            : state_(state::PREFIX)
            , cb_name_(cb_name)
            , buffer_{}
            , length_{0}
            , name_{} {}

        void parser::qname::reset() { buffer_.clear(); }

        bool parser::qname::put(std::uint8_t c) {

            //
            //  the whole data package is required for resolving pointers
            //
            buffer_.push_back(c);

            switch (state_) {
            case state::PREFIX: state_ = state_prefix(c); break;
            case state::NAME: state_ = state_name(c); break;
            // case state::TYPE_CLASS: state_ = state_type_class(c); break;
            default: BOOST_ASSERT_MSG(false, "illegal parser state"); break;
            }
            return state_ == state::PREFIX;
        }

        parser::qname::state parser::qname::state_prefix(std::uint8_t c) {

            name_.clear();
            if ((c & 0xC0) == 0xC0) {
                length_ = c & 0x3F; // ~0xC0
                return state::POINTER;
            } else if (c == 0) {
                //  end of this name
                buffer_.clear();
                return state::PREFIX;
            } else {
                length_ = c;
                return state::NAME;
            }
        }

        parser::qname::state parser::qname::state_name(std::uint8_t c) {
            //
            name_.push_back(static_cast<char>(c));
            if (--length_ == 0) {
                cb_name_(name_);
                return state::PREFIX;
            }
            return state_;
        }

        // parser::qname::state parser::qname::state_type_class(std::uint8_t c) {
        //     if (buffer_.size() == 4) {
        //         cb_tc_(buffer_[1] + (buffer_[0] << 8), buffer_[3] + (buffer_[2] << 8));
        //         buffer_.clear();
        //         //  next name
        //         return state::PREFIX;
        //     }
        //     return state_;
        // }

    } // namespace dns
} // namespace smf
