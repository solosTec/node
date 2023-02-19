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

#include <chrono>
#include <functional>
#include <type_traits>

namespace smf {
    namespace imega {

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
            parser();
        };

    } // namespace imega
} // namespace smf

#endif // SMF_IMEGA_PARSER_H
