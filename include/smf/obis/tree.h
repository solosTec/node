/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_OBIS_TREE_H
#define SMF_OBIS_TREE_H

#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/obis.h>
#include <cyng/obj/object.h>

#include <list>
#include <map>

namespace smf {
    namespace sml {

        /**
         * simple OBIS tree implementation
         */
        struct tree_node {

            using list_t = std::map<cyng::obis, tree_node>;

            tree_node();
            tree_node(cyng::object);

            cyng::object value_;
            list_t nodes_;
        };

        class tree {
          public:
            tree();

            /**
             * Set value of specified node
             * @return true if node was added otherwise false (for updated)
             */
            bool add(cyng::obis_path_t path, cyng::object value);

            /**
             * @return the value of the specified node
             */
            cyng::object find(cyng::obis_path_t path) const;

            /**
             * @return the tree below the specified path
             */
            tree get_subtree(cyng::obis_path_t path) const;

            /**
             * Returns 0 if path does not exists.
             *
             * @return number of elements in the specified node
             */
            std::size_t size(cyng::obis_path_t path) const;

            /**
             * convert an obis tree into a child list:
             * child_list : code, value, child_list
             */
            cyng::tuple_t to_child_list() const;

          private:
            tree(tree_node::list_t);

          private:
            tree_node::list_t nodes_;
        };

    } // namespace sml

} // namespace smf

#endif
