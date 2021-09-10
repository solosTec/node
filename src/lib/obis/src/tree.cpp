/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/obis/tree.h>
#include <smf/sml/value.hpp>

#include <cyng/io/ostream.h>
#include <cyng/obj/factory.hpp>
#include <cyng/obj/util.hpp>

#include <iostream>

#include <boost/assert.hpp>

namespace smf {
    namespace sml {

        namespace { //  static linkage

            cyng::tuple_t get_child_list(tree_node::list_t const &node) {
                if (node.size() == 1) {
                    auto const pos = node.begin();
                    return cyng::make_tuple(pos->first, pos->second.value_, get_child_list(pos->second.nodes_));
                }
                cyng::tuple_t child_list;

                //  std::map<cyng::obis, tree_node>
                std::transform(
                    std::begin(node),
                    std::end(node),
                    std::back_inserter(child_list),
                    [](tree_node::list_t::value_type const &node) {
                        return cyng::make_object(
                            (node.second.nodes_.empty())
                                ? cyng::make_tuple(node.first, to_value(node.second.value_), cyng::make_object())
                                : cyng::make_tuple(node.first, to_value(node.second.value_), get_child_list(node.second.nodes_)));
                    });
                return child_list;
            }

            /**
             * @return an empty tree if path does not exists
             */
            tree_node::list_t get_subtree(
                tree_node::list_t const &nodes,
                cyng::obis_path_t::const_iterator pos,
                cyng::obis_path_t::const_iterator path_end) {

                auto pos_nodes = nodes.find(*pos);
                if (pos_nodes != nodes.end()) {
                    ++pos;
                    return (pos == path_end) ? pos_nodes->second.nodes_ : get_subtree(pos_nodes->second.nodes_, pos, path_end);
                }
                //
                //  not found
                //
                return tree_node::list_t{};
            }

            cyng::object find(
                tree_node::list_t const &nodes,
                cyng::obis_path_t::const_iterator pos,
                cyng::obis_path_t::const_iterator path_end) {

                auto pos_nodes = nodes.find(*pos);
                if (pos_nodes != nodes.end()) {
                    ++pos;
                    return (pos == path_end) ? pos_nodes->second.value_ : find(pos_nodes->second.nodes_, pos, path_end);
                }
                //
                //  not found
                //
                return cyng::make_object();
            }

            /**
             * @return true if node was added otherwise false (for updated)
             */
            bool
            add(tree_node::list_t &nodes,
                cyng::obis_path_t::const_iterator pos,
                cyng::obis_path_t::const_iterator path_end,
                cyng::obis code,
                cyng::object value) {

                auto pos_nodes = nodes.find(code);
                if (pos == path_end) {
                    //
                    //  end of path
                    //
                    if (pos_nodes != nodes.end()) {
                        //
                        //  subtree/node exists
                        //
                        pos_nodes->second.value_ = std::move(value);
                    } else {
                        auto r = nodes.emplace(code, tree_node(value));
                        BOOST_ASSERT(r.second);
                        return r.second;
                    }

                } else {
                    //
                    //  continue
                    //
                    if (pos_nodes != nodes.end()) {
                        code = *pos++;
                        return add(pos_nodes->second.nodes_, pos, path_end, code, value);
                    } else {
                        auto r = nodes.emplace(code, tree_node());
                        BOOST_ASSERT(r.second);
                        code = *pos++;
                        return add(r.first->second.nodes_, pos, path_end, code, value);
                    }
                }

                return false;
            }

            bool
            add(tree_node::list_t &nodes,
                cyng::obis_path_t::const_iterator pos,
                cyng::obis_path_t::const_iterator path_end,
                cyng::object value) {

                if (pos != path_end) {

                    auto const code = *pos;
                    return add(nodes, ++pos, path_end, code, value);
                }
                return false;
            }
        } // namespace

        tree_node::tree_node()
            : value_()
            , nodes_() {}

        tree_node::tree_node(cyng::object obj)
            : value_(obj)
            , nodes_() {}

        tree::tree()
            : nodes_() {}

        tree::tree(tree_node::list_t nl)
            : nodes_(nl) {}

        bool tree::add(cyng::obis_path_t path, cyng::object value) {

            BOOST_ASSERT(!path.empty());
            return sml::add(nodes_, std::begin(path), std::end(path), value);
        }

        cyng::object tree::find(cyng::obis_path_t path) const {
            BOOST_ASSERT(!path.empty());

            return sml::find(nodes_, std::begin(path), std::end(path));
        }

        tree tree::get_subtree(cyng::obis_path_t path) const {
            return tree(sml::get_subtree(nodes_, std::begin(path), std::end(path)));
        }

        cyng::tuple_t tree::to_child_list() const { return get_child_list(nodes_); }

        std::size_t tree::size(cyng::obis_path_t path) const {

            return sml::get_subtree(nodes_, std::begin(path), std::end(path)).size();
        }

    } // namespace sml
} // namespace smf
