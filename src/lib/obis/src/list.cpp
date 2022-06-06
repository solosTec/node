/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/obis/db.h>
#include <smf/obis/list.h>

#include <cyng/io/ostream.h>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/tuple_cast.hpp>
#include <cyng/obj/value_cast.hpp>

#include <boost/assert.hpp>
#include <sstream>

namespace smf {
    namespace sml {

        std::pair<cyng::obis, cyng::tuple_t> get_list(cyng::object const &obj) {
            auto const tpl = cyng::container_cast<cyng::tuple_t>(obj);
            return get_list(tpl);
        }

        std::pair<cyng::obis, cyng::tuple_t> get_list(cyng::tuple_t const &tpl) {
            BOOST_ASSERT(tpl.size() == 3);
            if (tpl.size() == 3 && tpl.front().tag() == cyng::TC_OBIS) {
                return {cyng::value_cast(tpl.front(), cyng::obis()), cyng::container_cast<cyng::tuple_t>(tpl.back())};
            }
            return {{}, {}};
        }

        std::pair<cyng::obis, cyng::attr_t> get_attribute(cyng::object const &obj) {
            auto const list = cyng::container_cast<cyng::tuple_t>(obj);
            return get_attribute(list);
        }

        std::pair<cyng::obis, cyng::attr_t> get_attribute(cyng::tuple_t const &tpl) {
            BOOST_ASSERT(tpl.size() == 3);

            if (tpl.size() == 3 && tpl.front().tag() == cyng::TC_OBIS) {
                auto pos = std::next(tpl.begin()); //  second element
                return {cyng::value_cast(tpl.front(), cyng::obis()), cyng::value_cast(*pos, cyng::attr_t())};
            }
            return {{}, {}};
        }

        std::tuple<cyng::obis, cyng::attr_t, cyng::tuple_t> get_tree_values(cyng::object const &obj) {
            auto const list = cyng::container_cast<cyng::tuple_t>(obj);
            return get_tree_values(list);
        }

        std::tuple<cyng::obis, cyng::attr_t, cyng::tuple_t> get_tree_values(cyng::tuple_t const &tpl) {
            BOOST_ASSERT(tpl.size() == 3);
            if ((tpl.size() == 3) && (tpl.front().tag() == cyng::TC_OBIS)) {
                //
                //  we handle both cases: the value is stored as attribute or a tuple with two elements.
                //
                auto pos = std::next(tpl.begin()); //  second element
                if (pos->tag() == cyng::TC_ATTR) {
                    return cyng::tuple_cast<cyng::obis, cyng::attr_t, cyng::tuple_t>(tpl);
                } else if (pos->tag() == cyng::TC_TUPLE) {
                    auto [o, v, l] = cyng::tuple_cast<cyng::obis, cyng::tuple_t, cyng::tuple_t>(tpl);
                    if (v.size() == 2) {
                        return {o, {cyng::numeric_cast<std::size_t>(v.front(), 0), v.back()}, l};
                    }
                    return {o, cyng::attr_t(1, cyng::make_object()), l};
                }
            }
            return {{}, {}, {}};
        }

        bool has_child_list(cyng::tuple_t const &tpl) {
            // return (tpl.size() == 3) && (tpl.back().tag() == cyng::TC_TUPLE);
            if ((tpl.size() == 3) && (tpl.back().tag() == cyng::TC_TUPLE)) {
                auto const *ptr = cyng::object_cast<cyng::tuple_t>(tpl.back());
                return (ptr != nullptr && !ptr->empty());
            }
            return false;
        }

        void dump(std::ostream &os, cyng::tuple_t const &tpl, bool resolved, std::size_t depth) {
            if (tpl.size() == 3 && tpl.front().tag() == cyng::TC_OBIS) {
                auto const [o, a, l] = get_tree_values(tpl);
                //  show name if available
                if (resolved) {
                    os << std::string(depth * 2, ' ') << obis::get_name(o) << ": " << a.second << std::endl;
                } else {
                    os << std::string(depth * 2, ' ') << o << ": " << a.second << std::endl;
                }
                for (auto const &obj : l) {
                    auto c = cyng::container_cast<cyng::tuple_t>(obj);
                    dump(os, c, resolved, depth + 1);
                }
            } else {
                for (auto const &obj : tpl) {
                    auto c = cyng::container_cast<cyng::tuple_t>(obj);
                    dump(os, c, resolved, depth + 1);
                }
            }
        }

        void dump(std::ostream &os, cyng::tuple_t const &tpl, bool resolved) {
            //  start recursive call
            dump(os, tpl, resolved, 0);
        }

        std::string dump_child_list(cyng::tuple_t const &tpl, bool resolved) {
            std::stringstream ss;
            dump(ss, tpl, resolved);
            return ss.str();
        }

    } // namespace sml
} // namespace smf
