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
            return to_tuple<cyng::attr_t, cyng::tuple_t>(nodes_, [](cyng::obis code, cyng::attr_t attr) { return to_value(attr); });
        }

        cyng::prop_map_t obis_tree<cyng::attr_t>::to_prop_map() const { return sml::to_prop_map<cyng::attr_t>(nodes_); }

        cyng::prop_map_t obis_tree<cyng::attr_t>::to_prop_map(std::function<cyng::object(cyng::attr_t const &)> cb) const {
            return sml::to_prop_map<cyng::attr_t, cyng::object>(nodes_, cb);
        }

        cyng::prop_map_t obis_tree<cyng::attr_t>::to_prop_map_obj() const {
            return sml::to_prop_map<cyng::attr_t, cyng::object>(
                nodes_, [](cyng::attr_t const &attr) -> cyng::object { return attr.second; });
        }

    } // namespace sml
} // namespace smf
