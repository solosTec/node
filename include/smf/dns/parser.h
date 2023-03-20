/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_DNS_PARSER_H
#define SMF_DNS_PARSER_H

#include <smf/dns/header.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <string>
#include <type_traits>

namespace smf {
    namespace dns {
        class parser {
          private:
            enum class state { HEADER, DATA } state_;

          public:
            parser();

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

#ifdef _DEBUG
            msg get_msg() const;
#endif

          private:
            /**
             * read a single byte and update
             * parser state.
             * Implements the state machine
             */
            void put(char);

            state state_header(char c);
            state state_data(char c);

          private:
            std::size_t pos_;
            msg msg_;
        };

        auto parse_name(const uint8_t *pData) noexcept -> std::string;

        // https://github.com/MikhailGorobets/DNS/blob/master/src/dns.cpp
    } // namespace dns
} // namespace smf

#endif
