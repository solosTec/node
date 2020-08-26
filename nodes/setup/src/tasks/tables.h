/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SETUP_TABLES_H
#define NODE_SETUP_TABLES_H

#include <array>
#include <string>

namespace node
{
    struct tables
    {
        struct tbl_descr {
            std::string const name_;
            bool const cache_;
            bool const custom_; //  mapping to db with special rules
            inline tbl_descr(std::string name, bool cache, bool custom)
                : name_(name)
                , cache_(cache)
                , custom_(custom)
            {}
        };

        using array_t = std::array<tbl_descr, 11>;
        static const array_t list_;

        static array_t::const_iterator find(std::string);

        /**
         * If table is not in the list return value
         * is false (not custom).
         * 
         * @return true if custom flag is set.
         */
        static bool is_custom(std::string);
    };

}

#endif