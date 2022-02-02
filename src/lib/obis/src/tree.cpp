/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/obis/tree.hpp>
#include <smf/sml/value.hpp>

#include <cyng/io/ostream.h>
#include <cyng/obj/util.hpp>

#include <functional>
#include <iostream>

#include <boost/assert.hpp>

namespace smf {
    namespace sml {

        obis_tree<cyng::attr_t>::obis_tree() {}

        obis_tree<cyng::attr_t>::obis_tree(node_list_t &&nodes)
            : nodes_(std::move(nodes)) {}

        cyng::tuple_t obis_tree<cyng::attr_t>::to_child_list() const {
            return to_tuple<cyng::attr_t, cyng::tuple_t>(nodes_, [](cyng::attr_t attr) { return to_value(attr); });
        }

    } // namespace sml
} // namespace smf
