/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_OBIS_LIST_H
#define SMF_OBIS_LIST_H

#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/obis.h>

#include <iostream>

namespace smf {
    namespace sml {

        /**
         * Similiar to read_param_tree() but ignoring the attribute.
         *
         * - Test if the specified object is a tuple with 3 entries.
         * - Extract the first element as OBIS code
         * - Extract the last element as a child list (tuple)
         */
        std::pair<cyng::obis, cyng::tuple_t> get_list(cyng::object const &obj);
        std::pair<cyng::obis, cyng::tuple_t> get_list(cyng::tuple_t const &tpl);

        /**
         * Similiar to read_param_tree() but ignoring the child list
         *
         * - Test if the specified object is a tuple with 3 entries.
         * - Extract the first element as OBIS code
         * - Extract the second element as an attribute (attr_t)
         */
        std::pair<cyng::obis, cyng::attr_t> get_attribute(cyng::object const &obj);
        std::pair<cyng::obis, cyng::attr_t> get_attribute(cyng::tuple_t const &tpl);

        /**
         * dissect a tree
         */
        std::tuple<cyng::obis, cyng::attr_t, cyng::tuple_t> get_tree_values(cyng::object const &obj);
        std::tuple<cyng::obis, cyng::attr_t, cyng::tuple_t> get_tree_values(cyng::tuple_t const &tpl);

        /**
         * @return true if tpl contains three elements and the last one
         * is a tuple itself.
         */
        bool has_child_list(cyng::tuple_t const &tpl);

        /**
         * print the content as SML tree
         */
        void dump(std::ostream &, cyng::tuple_t const &tpl);
        std::string dump_child_list(cyng::tuple_t const &tpl);

    } // namespace sml

} // namespace smf

#endif
