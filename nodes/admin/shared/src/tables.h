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
            bool const pass_trough_;  
            inline tbl_descr(std::string name, bool custom, bool local, bool pass_trough)
                : name_(name)
                , custom_(custom)
                , local_(local)
                , pass_trough_(pass_trough)
            {}
        };

        using array_t = std::array<tbl_descr, 23>;
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

    struct channel 
    {
        struct rel
        {
            rel(std::string, std::string, std::string);

            const std::string table_;
            const std::string channel_;
            const std::string counter_;

            bool empty() const;
            bool has_counter() const;
        };

        /**
         * Search vector with table/channel relations for a table
         */
        static rel find_rel_by_table(std::string);
        static rel find_rel_by_channel(std::string);
        static rel find_rel_by_counter(std::string);

        /**
         * same size as tables::array_t
         */
        using array_t = std::array<rel, 23>;
        static array_t const rel_;

    };

    static_assert(std::tuple_size<tables::array_t>() == std::tuple_size<channel::array_t>(), "different array size for tables and relations");
}

#endif