/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_IMEGA_PARSER_H
#define SMF_IMEGA_PARSER_H

#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/version.h>

#include <chrono>
#include <functional>
#include <type_traits>

namespace smf {
    namespace imega {

        using cb_login = std::function<void(std::uint8_t, cyng::version, std::string, std::string)>;
        using cb_watchdog = std::function<void()>;
        /**
         * The bool value indicates and error
         */
        using cb_data = std::function<void(cyng::buffer_t &&, bool)>;

        class parser {
          private:
            /**
             *	Each session starts in login mode. The incoming
             *	data are expected to be an iMEGA login command. After a successful
             *	login, parser switch to stream mode.
             */
            enum class state : std::uint32_t {
                ERROR_,      //!>	error
                INITIAL,     //!<	initial - expect a '<'
                LOGIN,       //!<	gathering login data
                STREAM_MODE, //!<	parser in data mode
                ALIVE_A,     //!<	probe <ALIVE> watchdog
                ALIVE_L,     //!<	probe <ALIVE> watchdog
                ALIVE_I,     //!<	probe <ALIVE> watchdog
                ALIVE_V,     //!<	probe <ALIVE> watchdog
                ALIVE_E,     //!<	probe <ALIVE> watchdog
                ALIVE,       //!<	until '>'

                LAST,
            } state_;

          public:
            parser(cb_login, cb_watchdog, cb_data);

            parser(parser const &) = delete;

            /**
             * parse the specified range
             */
            template <typename I> std::size_t read(I start, I end) {
                static_assert(std::is_same_v<char, typename std::iterator_traits<I>::value_type>, "data type char expected");
                std::for_each(start, end, [this](char c) {
                    //
                    //	parse
                    //
                    this->put(c);
                });

                //
                //  input processed doesn't mean it's complete
                //
                if (post_processing()) {
                    buffer_.clear();
                    buffer_.reserve(64);
                }

                return std::distance(start, end);
            }

            /**
             * Clear buffer and reset internal state
             */
            void reset();

          private:
            /**
             *	Parse data
             */
            void put(char);

            /**
             * Probe if parsing is completed and
             * inform listener.
             */
            bool post_processing();

            state state_initial(char c);
            state state_login(char c);
            state state_stream_mode(char c);
            state state_alive_A(char c);
            state state_alive_L(char c);
            state state_alive_I(char c);
            state state_alive_V(char c);
            state state_alive_E(char c);
            state state_alive(char c);

          private:
            cb_login cb_login_;
            cb_watchdog cb_watchdog_;
            cb_data cb_data_;

            /**
             * temporary data
             */
            cyng::buffer_t buffer_;
        };

        std::uint8_t get_protocol(std::map<std::string, std::string> const &);
        cyng::version get_version(std::map<std::string, std::string> const &);
        std::string get_modulename(std::map<std::string, std::string> const &);
        std::string get_meterid(std::map<std::string, std::string> const &);

    } // namespace imega
} // namespace smf

#endif // SMF_IMEGA_PARSER_H
