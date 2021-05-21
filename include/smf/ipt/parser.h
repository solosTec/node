/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_PARSER_H
#define SMF_IPT_PARSER_H

#include <smf/ipt.h>
#include <smf/ipt/header.h>
#include <smf/ipt/scramble_key.h>
#include <smf/ipt/scrambler.hpp>

#include <cyng/obj/intrinsics/buffer.h>

#include <algorithm>
#include <iterator>
#include <type_traits>

#include <boost/assert.hpp>

namespace smf {
    namespace ipt {
        /** @brief state engine
         *
         * @code
                   +---------->
                   |          |
                   |  +----+  <--------+---+
                   |  |    |  |        |   |
                   |  |  +-v--v-----+  |   |
                   |  |  |  STREAM  |  |   |
                   |  |  +-+--+-----+  |   |
                   |  |    |  |        |   |
                   |  +----+  | ESC    |   |
                ESC|          |        |   |
                   |     +----v-----+  |   |
                   +-----+  ESC     |  |   |
                                 +----+-----+  |   |
                                          |        |   |
                                          |        |   |
                                 +----v-----+  |   |
                                 |  HEADER  +--+   |
                                 +----+-----+      |
                                          |            |
                                          |            |
                                 +----v------+     |
                                 |  BODY     +-----+
                                 +-----------+
         * @endcode
         *
         *	Whenever a IP-T command is complete a callback is triggered to process
         *	the generated instructions. This is to guarantee to stay in the
         *	correct state (since IP-T is statefull protocol). Especially the scrambled
         *	login requires special consideration, because of the  provided scramble key.
         *	In addition after a failed login no more instructions should be processed.
         */
        class parser {
            using scrambler_t = scrambler<char, scramble_key::SIZE::value>;

            /**
             * This enum stores the global state
             * of the parser. For each state there
             * are different helper variables mostly
             * declared in the private section of this class.
             */
            enum class state {
                STREAM,
                ESC,
                HEADER,
                DATA,
            } state_;

          public:
            using command_cb = std::function<void(header const &, cyng::buffer_t &&)>;
            using data_cb = std::function<void(cyng::buffer_t &&)>;

            parser(scramble_key const &, command_cb, data_cb);

            /**
             * parse the specified range
             */
            template <typename I> cyng::buffer_t read(I start, I end) {
                static_assert(std::is_same_v<char, typename std::iterator_traits<I>::value_type>, "data type char expected");

                cyng::buffer_t buffer;
                std::transform(start, end, std::back_inserter(buffer), [this](char c) {
                    //
                    //	Decode input stream
                    //
                    return this->put(scrambler_[c]);
                });

                post_processing();
                return buffer;
            }

            /**
             * Client has to set new scrambled after login request
             */
            void set_sk(scramble_key const &);

            /**
             * Reset parser (default scramble key)
             */
            void reset(scramble_key const &);

            /**
             * Clear internal state and all callback functions
             */
            void clear();

            /**
             * @return current scramble key
             */
            scramble_key::key_type const &get_sk() const;
            std::size_t get_scrambler_index() const;

          private:
            /**
             * read a single byte and update
             * parser state.
             * Implements the state machine
             */
            char put(char c);

            /**
             * Probe if parsing is completed and
             * inform listener.
             */
            void post_processing();

            state state_stream(char c);
            state state_esc(char c);
            state state_header(char c);
            state state_data(char c);

          private:
            scramble_key def_sk_; //!<	system wide default scramble key

            command_cb command_complete_;
            data_cb transmit_;

            /**
             * Decrypting data stream
             */
            scrambler_t scrambler_;

            /**
             * temporary data
             */
            cyng::buffer_t buffer_;

            //
            //	current header
            //
            header header_;
        };
    } // namespace ipt
} // namespace smf

#endif
