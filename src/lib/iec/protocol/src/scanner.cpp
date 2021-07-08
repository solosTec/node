/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/iec/scanner.h>

//#include <cyng/parse/string.h>

#include <iostream>
//#include <regex>

#include <boost/algorithm/string.hpp>
#include <boost/assert.hpp>

namespace smf {
    namespace iec {
        scanner::scanner(cb_f cb)
            : state_(state::VERSION)
            , cb_(cb)
            , line_(0)
            , id_() {}

        void scanner::put(char c) {

            //
            //  use <CR> and ignore <LF>
            //  This works for every state
            //
            if (c == 0x0a) {
                return;
            } else if (c == 0x0d) {
                ++line_;
                cb_(state_, line_, id_);
                return;
            }

            switch (state_) {
            case state::VERSION:
                if (STX == c) {
                    state_ = state::DATA;
                } else if (c > 32 && c < 127) {
                    id_.push_back(c);
                }
                break;
            case state::DATA:
                if (ETX == c) {
                    state_ = state::BBC;
                }
                break;
            case state::BBC:
                cb_(state_, line_, id_);
                reset();
                break;
            default:
                BOOST_ASSERT_MSG(false, "illegal parser state");
                break;
            }
        }

        void scanner::reset() {
            state_ = state::VERSION;
            line_ = 0;
            id_.clear();
        }

        std::string to_string(scanner::state s) {
            switch (s) {
            case scanner::state::VERSION:
                return "VERSION";
            case scanner::state::DATA:
                return "DATA";
            case scanner::state::BBC:
                return "CRC";
            default:
                break;
            }

            return "invalid IEC scanner state";
        }

    } // namespace iec
} // namespace smf
