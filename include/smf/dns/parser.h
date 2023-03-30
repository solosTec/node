/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_DNS_PARSER_H
#define SMF_DNS_PARSER_H

#include <smf/dns/msg.h>

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
            //    subparser
            class qname {
                enum class state { PREFIX, NAME, POINTER } state_;

              public:
                qname(std::function<void(std::string)>);
                void reset();
                bool put(std::uint8_t);

              private:
                state state_prefix(std::uint8_t c);
                state state_name(std::uint8_t c);

              private:
                std::function<void(std::string)> cb_name_;
                std::vector<std::uint8_t> buffer_;
                std::uint8_t length_; //!< length or offset
                std::string name_;
            };

          private:
            // parser
            enum class state { HEADER, QNAME, QTYPE, QCLASS } state_;

          public:
            parser();

            template <typename I> void read(I start, I end) {
                using value_type = typename std::iterator_traits<I>::value_type;
                static_assert(std::is_same_v<std::uint8_t, value_type>, "data type std::uint8_t expected");
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
            void put(std::uint8_t);

            state state_header(std::uint8_t c);
            state state_qname(std::uint8_t c);
            state state_qtype(std::uint8_t c);
            state state_qclass(std::uint8_t c);

            /**
             * callback
             */
            void name(std::string);

          private:
            std::size_t pos_;
            msg msg_;
            qname qname_;
        };

    } // namespace dns
} // namespace smf

#endif
