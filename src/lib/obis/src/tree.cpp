/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/obis/tree.h>

#include <cyng/io/ostream.h>
#include <cyng/obj/factory.hpp>

#include <iostream>

#include <boost/assert.hpp>

namespace smf {
    namespace obis {
        tree_node::tree_node()
            : value_()
            , nodes_() {}

        tree_node::tree_node(cyng::object obj)
            : value_(obj)
            , nodes_() {}

        void
        tree_node::add(cyng::obis_path_t::const_iterator pos_path, cyng::obis_path_t::const_iterator path_end, cyng::object value) {
            auto pos = pos_path++;
            if (pos_path == path_end) {
                auto pos_nodes = nodes_.find(*pos);
                if (pos_nodes != nodes_.end()) {
                    std::cout << "append " << *pos << " = " << value << std::endl;
                    pos_nodes->second.value_ = std::move(value);
                } else {
                    std::cout << "new " << *pos << " = " << value << std::endl;
                    nodes_.emplace(*pos, tree_node(value));
                }
            } else {
                auto pos_nodes = nodes_.find(*pos);
                if (pos_nodes != nodes_.end()) {
                    //
                    //  update
                    //
                    std::cout << "next " << *pos << std::endl;
                    pos_nodes->second.add(pos_path, path_end, value);
                } else {
                    //
                    //  insert
                    //
                    std::cout << "insert " << *pos << std::endl;
                    auto r = nodes_.emplace(*pos, tree_node());
                    BOOST_ASSERT(r.second);
                    if (r.second) {
                        r.first->second.add(pos_path, path_end, value);
                    }
                }
            }
        }

        cyng::object tree_node::find(cyng::obis_path_t::const_iterator pos_path, cyng::obis_path_t::const_iterator path_end) const {
            auto pos = pos_path++;
            std::cout << "lookup " << *pos << std::endl;
            auto pos_nodes = nodes_.find(*pos);
            if (pos_nodes != nodes_.end()) {
                return (pos_path == path_end) ? pos_nodes->second.value_ : pos_nodes->second.find(pos_path, path_end);
            }
            //
            //  not found
            //
            return cyng::make_object();
        }

        tree::tree()
            : nodes_() {}

        void tree::add(cyng::obis_path_t path, cyng::object value) {

            BOOST_ASSERT(!path.empty());

            //
            //  start
            //
            auto pos_path = path.begin();
            if (pos_path != path.end()) {
                auto pos_nodes = nodes_.find(*pos_path);
                if (pos_nodes != nodes_.end()) {
                    //
                    //  exists
                    //
                    auto pos = pos_path++;
                    if (pos_path == path.end()) {
                        //
                        //  update
                        //
                        std::cout << "update" << std::endl;
                    } else {
                        //
                        //  next node
                        //
                        std::cout << "next " << *pos << std::endl;
                        pos_nodes->second.add(pos_path, path.end(), value);
                    }

                } else {
                    //
                    //  doesn't exist
                    //
                    auto pos = pos_path++;
                    if (pos_path == path.end()) {
                        //
                        //  insert
                        //
                        std::cout << "insert " << *pos << std::endl;

                    } else {
                        //
                        //  next node
                        //
                        std::cout << "emplace " << *pos << std::endl;
                        auto r = nodes_.emplace(*pos, tree_node());
                        BOOST_ASSERT(r.second);
                        if (r.second) {
                            r.first->second.add(pos_path, path.end(), value);
                        }
                    }
                }
            }
        }

        cyng::object tree::find(cyng::obis_path_t path) const {
            BOOST_ASSERT(!path.empty());

            //
            //  start
            //
            auto pos_path = path.begin();
            if (pos_path != path.end()) {
                auto pos_nodes = nodes_.find(*pos_path);
                if (pos_nodes != nodes_.end()) {
                    //
                    //  exists
                    //
                    auto pos = pos_path++;
                    if (pos_path == path.end()) {
                        //
                        //  end of path
                        //
                        return pos_nodes->second.value_;
                    } else {
                        //
                        //  walk down
                        //
                        return pos_nodes->second.find(pos_path, path.end());
                    }
                }
            }
            return cyng::make_object();
        }

    } // namespace obis
} // namespace smf
