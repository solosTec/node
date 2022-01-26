/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_SELECT_H
#define SMF_SML_SELECT_H

#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/obis.h>
#include <cyng/obj/object.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace smf {
    namespace sml {
        //
        //  select device information from a get proc parameter response.
        //
        // void select_devices(cyng::tuple_t const &);

        void select(cyng::tuple_t const &, cyng::obis root, std::function<void(cyng::prop_map_t const &, std::size_t)> cb);
        void collect(cyng::tuple_t const &tpl, cyng::obis o, std::function<void(cyng::prop_map_t const &)> cb);

        /**
         * Collect also nested data like in ROOT_PUSH_OPERATIONS (81 81 c7 8a 01 ff).
         */
        void collect(
            cyng::tuple_t const &tpl,
            cyng::obis_path_t,
            std::function<void(cyng::obis_path_t const &, cyng::object const &)> cb);

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
         * dissecta a tree
         */
        std::tuple<cyng::obis, cyng::attr_t, cyng::tuple_t> get_tree_values(cyng::object const &obj);
        std::tuple<cyng::obis, cyng::attr_t, cyng::tuple_t> get_tree_values(cyng::tuple_t const &tpl);

        /**
         * @return true if tpl contains three elements and the last one
         * is a tuple itself.
         */
        bool has_child_list(cyng::tuple_t const &tpl);

    } // namespace sml
} // namespace smf
#endif
