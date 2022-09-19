/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_RADIO_PARSER_H
#define SMF_MBUS_RADIO_PARSER_H

#include <smf/mbus/radio/header.h>

#include <cyng/obj/intrinsics/buffer.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>

namespace smf {
    namespace mbus {
        namespace radio {
            class parser {
              public:
                using cb_t = std::function<void(header const &, tplayer const &, cyng::buffer_t const &)>;

              private:
                enum class state { HEADER, TPL_SHORT, TPL_LONG, DATA } state_;

              public:
                parser(cb_t);

                /**
                 * parse the specified range
                 */
                template <typename I> void read(I start, I end) {
                    using value_type = typename std::iterator_traits<I>::value_type;
                    static_assert(std::is_same_v<char, value_type>, "data type char expected");
                    std::for_each(start, end, [this](value_type c) {
                        //
                        //	parse
                        //
                        this->put(c);
                    });
                }

              private:
                /**
                 * read a single byte and update
                 * parser state.
                 * Implements the state machine
                 */
                void put(char);

                state state_header(char c);
                state state_tpl(char c);
                state state_data(char c);

              private:
                cb_t cb_;
                std::size_t pos_;
                header header_;
                tplayer tpl_;   //!< Transport Layer
                cyng::buffer_t payload_;
            };
        } // namespace radio
    }     // namespace mbus
} // namespace smf

#endif
