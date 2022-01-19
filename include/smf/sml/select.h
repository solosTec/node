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
        void select_devices(cyng::tuple_t const &);

        /**
         * Similiar to read_param_tree() but ignoring the attribute.
         *
         * - Test if the specified object is a tuple with 3 entries.
         * - Extract the first element as OBIS code
         * - Extract the last element as a child list (tuple)
         */
        std::tuple<cyng::obis, cyng::tuple_t> get_list(cyng::object const &obj);
        std::tuple<cyng::obis, cyng::tuple_t> get_list(cyng::tuple_t const &tpl);

        /**
         * Similiar to read_param_tree() but ignoring the child list
         *
         * - Test if the specified object is a tuple with 3 entries.
         * - Extract the first element as OBIS code
         * - Extract the second element as an attribute (attr_t)
         */
        std::tuple<cyng::obis, cyng::attr_t> get_attribute(cyng::object const &obj);
        std::tuple<cyng::obis, cyng::attr_t> get_attribute(cyng::tuple_t const &tpl);
    } // namespace sml
} // namespace smf
#endif
