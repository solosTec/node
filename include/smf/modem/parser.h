/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MODEM_PARSER_H
#define SMF_MODEM_PARSER_H

#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/intrinsics/container.h>

#include <chrono>
#include <functional>
#include <type_traits>

namespace smf {
    namespace modem {
        /**
         *	Whenever a modem/AT command is complete a callback is triggered to process
         *	the generated instructions. This is to guarantee to stay in the
         *	correct state.
         *
         *	Defines the modem protocol state machine.
         * The modem state machine has three states: The command, data and
         * escape state. The machine starts in the command mode. After receiving
         * the "ATA" command ist switches into the data mode. An escape symbol
         * (+++) followed by an "ATH" switches back into command mode.
         * @code

              +
              |
              |
              |
          v
         +----+----- ----------------------------------------------------+
         |          -                                                    |
         |   start  |                                                    |
         |          v                                                    |
         |  +-------+---+  +--- --------------------------------------+  |
         |  |           |  |   -                                      |  |
         |  | command   |  |   |       stream                         |  |
         |  |           |  |   v                                      |  |
         |  |           |  | +-+----------------+ +-----------------+ |  |
         |  |           |  | |                  | |                 | |  |
         |  |           |  | | data             | | escape          | |  |
         |  |           |  | |                  | |                 | |  |
         |  |           |  | |                  | |                 | |  |
         |  |           |  | |                  | |                 | |  |
         |  |           |  | |                  | |                 | |  |
         |  |           |  | |                  | |                 | |  |
         |  |           |  | |                  | |                 | |  |
         |  |           |  | +------------------+ +-----------------+ |  |
         |  |           |  |                                          |  |
         |  +-----------+  +------------------------------------------+  |
         |                                                               |
         +---------------------------------------------------------------+
         * @endcode
         *
         * @see https://en.wikipedia.org/wiki/Hayes_command_set
         */

        class parser {
          public:
            using command_cb = std::function<void(std::string &&)>;
            using data_cb = std::function<void(cyng::buffer_t &&)>;

          private:
            /**
             * This enum stores the global state
             * of the parser. For each state there
             * are different helper variables mostly
             * declared in the private section of this class.
             */
            enum class state {
                COMMAND,
                STREAM,
                ESC,
                ERROR_,
            } state_;

            struct esc {
                esc();
                esc(esc const &);
                bool is_on_time(std::chrono::milliseconds) const;
                bool is_complete() const;

                esc &operator++();
                esc operator++(int);

                std::chrono::system_clock::time_point start_;
                std::size_t counter_;
            };

          public:
            /**
             * @param cb this function is called, when parsing is complete
             */
            parser(command_cb, data_cb, std::chrono::milliseconds guard_time);

            /**
             * The destructor is required since the unique_ptr
             * uses an incomplete type.
             */
            virtual ~parser();

            /**
             * parse the specified range
             */
            template <typename I> auto read(I start, I end) -> typename std::iterator_traits<I>::difference_type {
                std::for_each(start, end, [this](char c) {
                    //
                    //	Decode input stream
                    //
                    std::size_t limit = 16;
                    while (!this->put(c) && (limit-- != 0)) {
                        ;
                    }
                });

                //
                //  transmission complete in data mode
                //
                post_processing();
                return std::distance(start, end);
            }

            /**
             * transit to stream mode
             */
            void set_stream_mode();

            /**
             * transit to command mode
             */
            void set_cmd_mode();

            /**
             * Clear internal state and all callback function
             */
            void clear();

          private:
            /**
             * read a single byte and update
             * parser state.
             * Implements the state machine
             */
            bool put(char);

            /**
             * Probe if parsing is completed and
             * inform listener.
             */
            void post_processing();

            std::pair<state, bool> state_command(char);
            std::pair<state, bool> state_stream(char);
            std::pair<state, bool> state_esc(char);

          private:
            /**
             * call this method if parsing is complete
             */
            command_cb command_cb_;
            data_cb data_cb_;

            std::chrono::milliseconds const guard_time_;

            /**
             * temporary data
             */
            cyng::buffer_t buffer_;

            /**
             * escape state
             */
            esc esc_;
        };

        constexpr bool is_eol(char c) { return (c == '\n') || (c == '\r'); }
        constexpr bool is_esc(char c) { return (c == '+'); }

    } // namespace modem
} // namespace smf

#endif // NODE_MODEM_PARSER_H
