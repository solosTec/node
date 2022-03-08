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
#include <cyng/obj/intrinsics/obis.h>

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

        cyng::buffer_t set_crc16(cyng::buffer_t &&buffer);

        /**
         * Add SML trailer and tail
         */
        cyng::buffer_t boxing(std::vector<cyng::buffer_t> const &);
        cyng::buffer_t boxing(cyng::buffer_t &&);

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

        /**
         * Generate an empty tree
         */
        cyng::tuple_t tree_empty(cyng::obis code);

        /**
         * Generate a tree with an parameter entry and an empty
         * child list
         */
        cyng::tuple_t tree_param(cyng::obis code, cyng::tuple_t param);
        cyng::tuple_t tree_param(cyng::obis code, cyng::attr_t attr);

        /**
         * Generate a tree with an child list entry and an empty
         * parameter entry
         *
         * @param code parameter name
         * @param child_list list of SML_Tree
         */
        cyng::tuple_t tree_child_list(cyng::obis code, cyng::tuple_t child_list);

        /**
         * Generate a tree with an child list entry and an empty
         * parameter entry
         *
         * @param code parameter name
         * @param list list of SML_Tree. Contains tuples with 3 (or 0) elements
         */
        cyng::tuple_t tree_child_list(cyng::obis code, std::initializer_list<cyng::tuple_t> list);

        /**
         * Generate a SML_ListEntry
         */
        cyng::tuple_t list_entry(cyng::obis code, std::uint32_t status, std::uint8_t unit, std::int8_t scaler, cyng::object value);

    } // namespace sml
} // namespace smf
#endif
