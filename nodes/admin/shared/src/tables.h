/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_ADMIN_TABLES_H
#define NODE_ADMIN_TABLES_H

#include <array>
#include <string>

namespace node
{
    struct tables
    {
        struct tbl_descr {
            std::string const name_;
            bool const custom_;
            bool const local_;  //  internal dash table
            inline tbl_descr(std::string name, bool custom, bool local)
                : name_(name)
                , custom_(custom)
                , local_(local)
            {}
        };

        using array_t = std::array<tbl_descr, 18>;
        static const array_t list_;

        static array_t::const_iterator find(std::string);

        /**
         * If table is not in the list return value
         * is false (not custom).
         * 
         * @return true if custom flag is set.
         */
        static bool is_custom(std::string);
        static bool is_local(std::string);
    };

}

#endif