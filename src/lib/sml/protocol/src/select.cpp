#include <smf/obis/defs.h>
#include <smf/obis/list.h>
#include <smf/sml/select.h>

#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/container_cast.hpp>
#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/tuple_cast.hpp>

#ifdef _DEBUG_SML
#include <cyng/io/ostream.h>
#include <iostream>
#endif
#ifdef _DEBUG
#include <cyng/io/ostream.h>
#include <cyng/io/serialize.h>
#endif
#include <boost/assert.hpp>

namespace smf {
    namespace sml {

        namespace {
            std::size_t select(
                cyng::tuple_t const &tpl,
                cyng::obis root,
                std::size_t idx,
                std::function<void(cyng::prop_map_t const &, std::size_t)> cb) {
                auto depth = cyng::depth(root);
                if (depth > 0) {
                    for (auto const &v : tpl) {
                        auto const [code, list] = get_list(v);
                        idx = select(list, code, idx, cb);
                    }
                } else {
                    //  data level
                    cyng::prop_map_t om;
                    for (auto const &v : tpl) {
                        auto const [c, attr] = get_attribute(v);
                        om.emplace(c, attr.second);
                    }
                    // std::cerr << '#' << idx << " - " << om << std::endl;
                    cb(om, idx);
                    ++idx;
                }
                return idx;
            }
        } // namespace

        void select(cyng::tuple_t const &tpl, cyng::obis root, std::function<void(cyng::prop_map_t const &, std::size_t)> cb) {
            select(tpl, root, 0, cb);
        }

        void collect(cyng::tuple_t const &tpl, cyng::obis o, std::function<void(cyng::prop_map_t const &)> cb) {
            cyng::prop_map_t om;
            for (auto const &v : tpl) {
                auto const [c, attr] = get_attribute(v);
                om.emplace(c, attr.second);
            }
            // std::cerr << '#' << idx << " - " << om << std::endl;
            cb(om);
        }

        void collect(
            cyng::tuple_t const &tpl,
            cyng::obis_path_t p,
            std::function<void(cyng::obis_path_t const &, cyng::attr_t const &)> cb) {
            for (auto const &v : tpl) {
                // std::cerr << "collect: " << p << " - " << v << std::endl;
                auto const [o, a, l] = get_tree_values(v);
                if (a.first != 0) {
                    cb(cyng::append(p, o), a);
                }
                collect(l, cyng::append(p, o), cb);
            }
        }

        cyng::prop_map_t compress(cyng::tuple_t const &tpl) {
            //  replace attribute by it's object
            return transform(tpl, [](cyng::attr_t const &attr) -> cyng::object { return attr.second; });
        }

        namespace {
            cyng::prop_map_t
            transform(cyng::tuple_t const &tpl, std::function<cyng::object(cyng::attr_t const &)> cb, std::size_t depth) {
                cyng::prop_map_t r;
                for (auto const &v : tpl) {
                    auto const [o, a, l] = get_tree_values(v);
                    // std::cerr << "transform #" << depth << std::string(depth * 2, '-') << o << ": " << a << std::endl;
                    if (!l.empty()) {
                        r.emplace(o, cyng::make_object(transform(l, cb, depth + 1)));
                    } else {
                        r.emplace(o, cb(a));
                    }
                }
                return r;
            }
        } // namespace

        cyng::prop_map_t transform(cyng::tuple_t const &tpl, std::function<cyng::object(cyng::attr_t const &)> cb) {
            return transform(tpl, cb, 0);
        }

    } // namespace sml
} // namespace smf
