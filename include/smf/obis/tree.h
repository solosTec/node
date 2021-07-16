/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_OBIS_TREE_H
#define SMF_OBIS_TREE_H

#include <cyng/obj/intrinsics/obis.h>
#include <cyng/obj/object.h>

#include <list>
#include <map>

namespace smf {
    namespace obis {

        /**
         * simple OBIS tree implementation
         */
        struct tree_node {

            using list_t = std::map<cyng::obis, tree_node>;

            tree_node();
            tree_node(cyng::object);

            void add(cyng::obis_path_t::const_iterator, cyng::obis_path_t::const_iterator, cyng::object);
            cyng::object find(cyng::obis_path_t::const_iterator, cyng::obis_path_t::const_iterator) const;

            cyng::object value_;
            list_t nodes_;
        };

        class tree {
          public:
            tree();
            void add(cyng::obis_path_t path, cyng::object value);
            cyng::object find(cyng::obis_path_t path) const;

          private:
            tree_node::list_t nodes_;
        };
    } // namespace obis
} // namespace smf

#endif
