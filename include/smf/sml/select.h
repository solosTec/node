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
        //  select (device) information from a get proc parameter response.
        //
        void select(cyng::tuple_t const &, cyng::obis root, std::function<void(cyng::prop_map_t const &, std::size_t)> cb);

        /**
         * @brief Recursive traversing a child list to produce a property map.
         *
         * Collect all attribute values. Child lists are inserted as a tree.
         */
        void collect(cyng::tuple_t const &tpl, std::function<void(cyng::prop_map_t const &)> cb);

        /**
         * Collect also nested data like in ROOT_PUSH_OPERATIONS (81 81 c7 8a 01 ff).
         *
         * @param cb called for every valid attribute
         */
        void collect(
            cyng::tuple_t const &tpl,
            cyng::obis_path_t,
            std::function<void(cyng::obis_path_t const &, cyng::attr_t const &)> cb);

        /**
         * replace all attributes by it's object
         */
        cyng::prop_map_t compress(cyng::tuple_t const &tpl);

        /**
         * convert all attributes to an object.
         * assuming the attributes and child lists are exclusive.
         */
        cyng::prop_map_t transform(cyng::tuple_t const &tpl, std::function<cyng::object(cyng::attr_t const &)>);

    } // namespace sml
} // namespace smf
#endif
