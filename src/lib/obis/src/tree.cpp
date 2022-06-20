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

        namespace {
            /**
             * @param nodes recursive structure of obis nodes
             * @param f transformer function for value T
             */
            cyng::tuple_t to_sml(
                typename obis_node<cyng::attr_t>::list_t const &nodes,
                std::function<cyng::tuple_t(cyng::obis, cyng::attr_t)> f) {
                cyng::tuple_t tpl;
                // BOOST_ASSERT(!nodes.empty());
                std::transform(
                    std::begin(nodes),
                    std::end(nodes),
                    std::back_inserter(tpl),
                    [=](typename obis_node<cyng::attr_t>::list_t::value_type const &node) {
                        if (node.second.nodes_.empty()) {
                            return cyng::make_object(cyng::make_tuple(node.first, f(node.first, node.second.value_), cyng::null{}));
                        }

                        auto tmp = to_sml(node.second.nodes_, f);

                        // std::cout << "..." << node.second.value_ << std::endl;
                        return cyng::make_object(
                            (node.second.value_.first == 0)
                                // make_param_tree()
                                ? cyng::make_tuple(node.first, cyng::null{}, tmp)
                                : cyng::make_tuple(node.first, f(node.first, node.second.value_), tmp));
                    });
                return tpl;
            }
        } // namespace

        obis_tree<cyng::attr_t>::obis_tree() {}

        obis_tree<cyng::attr_t>::obis_tree(node_list_t &&nodes)
            : nodes_(std::move(nodes)) {}

        cyng::tuple_t obis_tree<cyng::attr_t>::to_child_list() const {
            return to_tuple<cyng::attr_t, cyng::tuple_t>(
                nodes_, [](cyng::obis code, cyng::attr_t attr) -> cyng::tuple_t { return to_value(attr); });
        }

        cyng::tuple_t obis_tree<cyng::attr_t>::to_sml() const {
            return smf::sml::to_sml(nodes_, [](cyng::obis code, cyng::attr_t attr) -> cyng::tuple_t { return to_value(attr); });
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
