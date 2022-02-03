/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_OBIS_TREE_HPP
#define SMF_OBIS_TREE_HPP

#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/obis.h>
#include <cyng/obj/object.h>

#include <list>
#include <map>

namespace smf {
    namespace sml {

        /**
         * generic obis node consists of a value and an optional list
         * of more values identified by an obis code.
         */
        template <typename T> struct obis_node {

            using list_t = std::map<cyng::obis, obis_node>;

            obis_node()
                : value_{} {}
            obis_node(T value)
                : value_(value) {}

            T value_;
            list_t nodes_;
        };

        template <typename T>
        T find(
            typename obis_node<T>::list_t const &nodes,
            cyng::obis_path_t::const_iterator pos,
            cyng::obis_path_t::const_iterator path_end) {

            auto pos_nodes = nodes.find(*pos);
            if (pos_nodes != nodes.end()) {
                ++pos;
                return (pos == path_end) ? pos_nodes->second.value_ : find<T>(pos_nodes->second.nodes_, pos, path_end);
            }
            //
            //  not found
            //
            return T{};
        }
        /**
         * @return true if node was added otherwise false (for updated)
         */
        template <typename T>
        bool
        add(typename obis_node<T>::list_t &nodes,
            cyng::obis_path_t::const_iterator pos,
            cyng::obis_path_t::const_iterator path_end,
            cyng::obis code,
            T value) {

            using node_t = obis_node<T>;

            auto pos_nodes = nodes.find(code);
            if (pos == path_end) {
                //  end of path
                if (pos_nodes != nodes.end()) {
                    //  subtree/node exists
                    pos_nodes->second.value_ = std::move(value);
                } else {
                    auto r = nodes.emplace(code, node_t(value));
                    return r.second;
                }
            } else {
                //  continue
                if (pos_nodes != nodes.end()) {
                    code = *pos++;
                    return add<T>(pos_nodes->second.nodes_, pos, path_end, code, value);
                } else {
                    auto r = nodes.emplace(code, obis_node<T>());
                    BOOST_ASSERT(r.second);
                    code = *pos++;
                    return add<T>(r.first->second.nodes_, pos, path_end, code, value);
                }
            }
            return false;
        }

        template <typename T>
        bool
        add(typename obis_node<T>::list_t &nodes,
            cyng::obis_path_t::const_iterator pos,
            cyng::obis_path_t::const_iterator path_end,
            T value) {

            if (pos != path_end) {

                auto const code = *pos;
                return add<T>(nodes, ++pos, path_end, code, value);
            }
            return false;
        }

        /**
         * @return an empty tree if path does not exists
         */
        template <typename T>
        auto get_subtree(
            typename obis_node<T>::list_t const &nodes,
            cyng::obis_path_t::const_iterator pos,
            cyng::obis_path_t::const_iterator path_end) -> typename obis_node<T>::list_t {

            auto pos_nodes = nodes.find(*pos);
            if (pos_nodes != nodes.end()) {
                ++pos;
                return (pos == path_end) ? pos_nodes->second.nodes_ : get_subtree<T>(pos_nodes->second.nodes_, pos, path_end);
            }
            //
            //  not found
            //
            return typename obis_node<T>::list_t{};
        }

        template <typename T> cyng::tuple_t to_tuple(typename obis_node<T>::list_t const &nodes) {
            cyng::tuple_t tpl;

            std::transform(
                std::begin(nodes),
                std::end(nodes),
                std::back_inserter(tpl),
                [](typename obis_node<T>::list_t::value_type const &node) {
                    return cyng::make_object(cyng::make_tuple(node.first, node.second.value_, to_tuple<T>(node.second.nodes_)));
                });
            return tpl;
        }

        template <typename T, typename R>
        cyng::tuple_t to_tuple(typename obis_node<T>::list_t const &nodes, std::function<R(T)> f) {
            cyng::tuple_t tpl; //  result

            std::transform(
                std::begin(nodes), std::end(nodes), std::back_inserter(tpl), [=](obis_node<T>::list_t::value_type const &node) {
                    return cyng::make_object(
                        cyng::make_tuple(node.first, f(node.second.value_), to_tuple<T, R>(node.second.nodes_, f)));
                });
            return tpl;
        }

        /**
         * A tree implementation with obis codes as key.
         */
        template <typename T> class obis_tree {
          public:
            using node_t = obis_node<T>;
            using node_list_t = node_t::list_t;

          public:
            obis_tree()
                : nodes_() {}

            /**
             * Set value of specified node
             * @return true if node was added otherwise false (for updated)
             */
            bool add(cyng::obis_path_t path, T value) {
                BOOST_ASSERT(!path.empty());
                return sml::add<T>(nodes_, std::begin(path), std::end(path), value);
            }

            /**
             * @return the value of the specified node
             */
            T find(cyng::obis_path_t path) const { return sml::find<T>(nodes_, std::begin(path), std::end(path)); }

            /**
             * @return the tree below the specified path
             */
            obis_tree get_subtree(cyng::obis_path_t path) const {
                return sml::get_subtree<T>(nodes_, std::begin(path), std::end(path));
            }

            /**
             * Returns 0 if path does not exists.
             *
             * @return number of elements in the specified node
             */
            std::size_t size(cyng::obis_path_t path) const {
                return sml::get_subtree<T>(nodes_, std::begin(path), std::end(path)).size();
            }

            /**
             * convert an obis tree into a child list:
             * child_list : code, value, child_list
             */
            cyng::tuple_t to_tuple() const { return sml::to_tuple<T>(nodes_); }

            template <typename R> cyng::tuple_t to_tuple(std::function<R(T)> f) const { return sml::to_tuple<T, R>(nodes_, f); }

          private:
            obis_tree(node_list_t &&nodes)
                : nodes_(std::move(nodes)) {}

          private:
            node_list_t nodes_;
        };

        /**
         * specialized for cyng::attr_t
         */
        template <> class obis_tree<cyng::attr_t> {
          public:
            using node_t = obis_node<cyng::attr_t>;
            using node_list_t = node_t::list_t;

          public:
            obis_tree();

            bool add(cyng::obis_path_t path, cyng::attr_t value) {
                BOOST_ASSERT(!path.empty());
                return sml::add<cyng::attr_t>(nodes_, std::begin(path), std::end(path), value);
            }

            /**
             * @return the value of the specified node
             */
            cyng::attr_t find(cyng::obis_path_t path) const {
                return sml::find<cyng::attr_t>(nodes_, std::begin(path), std::end(path));
            }

            /**
             * @return the tree below the specified path
             */
            obis_tree<cyng::attr_t> get_subtree(cyng::obis_path_t path) const {
                return sml::get_subtree<cyng::attr_t>(nodes_, std::begin(path), std::end(path));
            }

            /**
             * Returns 0 if path does not exists.
             *
             * @return number of elements in the specified node
             */
            std::size_t size(cyng::obis_path_t path) const {
                return sml::get_subtree<cyng::attr_t>(nodes_, std::begin(path), std::end(path)).size();
            }

            /**
             * convert an obis tree into a child list:
             * child_list : code, value, child_list
             */
            cyng::tuple_t to_child_list() const;

          private:
            obis_tree(node_list_t &&nodes);

          private:
            node_list_t nodes_;
        };

        using tree = obis_tree<cyng::attr_t>;

    } // namespace sml

} // namespace smf

#endif
