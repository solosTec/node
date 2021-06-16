/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_MSG_H
#define SMF_SML_MSG_H

#include <smf/sml.h>

#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/intrinsics/container.h>

#include <cstdint>
#include <functional>

namespace smf {
    namespace sml {

        /**
         * list of messages ready to serialize (see to_sml() function)
         */
        using messages_t = std::vector<cyng::tuple_t>;

        /**
         * Convert a list of messages into a SML message
         */
        cyng::buffer_t to_sml(sml::messages_t const &);

        /**
         * Add SML trailer and tail
         */
        cyng::buffer_t boxing(std::vector<cyng::buffer_t> const &);

        /**
         * generic function to produce a SML message
         */
        cyng::tuple_t make_message(
            std::string trx,
            std::uint8_t group_no,
            std::uint8_t abort_code,
            msg_type type,
            cyng::tuple_t body,
            std::uint16_t);

        /**
         * generic function to produce a SML message using a function to
         * create the message body.
         */
        template <typename... Args>
        cyng::tuple_t make_message(
            std::string trx,
            std::uint8_t group_no,
            std::uint8_t abort_code,
            msg_type type,
            std::function<cyng::tuple_t(Args...)> f,
            std::uint16_t crc,
            Args &&...args) {
            return make_message(trx, group_no, abort_code, f(std::forward<Args>(args)...), crc);
        }

    } // namespace sml
} // namespace smf
#endif
