/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IEC_SCANNER_H
#define SMF_IEC_SCANNER_H

#include <smf/iec/defs.h>

#include <cyng/obj/intrinsics/edis.h>
#include <cyng/obj/intrinsics/obis.h>

#include <algorithm>
#include <functional>
#include <string>

namespace smf {
    namespace iec {
        /** @brief simple scanner of the iec 62056-21 protocol
         *
         * /device id<CR><LF>
         * STX
         * data
         * !<CR><LF>
         * ETX
         * BBC (all data between STX and ETX)
         */
        class scanner {
          public:
            enum class state {
                VERSION,
                DATA,
                BBC, //  crc
            } state_;

            /**
             * The second parameter contains the line number.
             * Each <CR><LF> (0x0d, 0x0a) pair increases the line number.
             */
            using cb_f = std::function<void(state, std::size_t, std::string)>;

          public:
            scanner(cb_f);

            /**
             * parse the specified range
             */
            template <typename I> void read(I start, I end) {
                static_assert(std::is_same_v<char, typename std::iterator_traits<I>::value_type>, "data type char expected");
                std::for_each(start, end, [this](char c) {
                    //
                    //	parse
                    //
                    this->put(c);
                });
            }

            /**
             * read a single byte and update
             * parser state.
             * Implements the state machine
             */
            void put(char);
            void reset();

          private:
            cb_f cb_;
            std::size_t line_;
            std::string id_; //!< device id
        };

        std::string to_string(scanner::state);

    } // namespace iec
} // namespace smf

#endif
