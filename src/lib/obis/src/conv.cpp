/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/obis/conv.h>

#include <smf/obis/db.h>

#include <cyng/obj/util.hpp>
#include <cyng/obj/value_cast.hpp>
#include <cyng/parse/buffer.h>
#include <cyng/parse/string.h>

#include <algorithm>
#include <iterator>

#include <boost/assert.hpp>

namespace smf {
    namespace obis {

        std::vector<std::string> to_str_vector(cyng::obis_path_t const &path, bool translate) {
            std::vector<std::string> vec;
            std::transform(path.begin(), path.end(), std::back_inserter(vec), [&](cyng::obis code) {
                return (translate) ? get_name(code) : cyng::to_string(code);
            });
            return vec;
        }

        std::string to_string(cyng::obis_path_t const &path, bool translate, char sep) {
            auto const vec = to_str_vector(path, translate);
            std::stringstream ss;
            bool init = false;
            for (auto const &s : vec) {
                if (!init) {
                    init = true;
                } else {
                    ss << sep;
                }
                ss << s;
            }
            return ss.str();
        }

        cyng::obis_path_t to_obis_path(std::vector<std::string> const &vec) {
            cyng::obis_path_t path;
            path.reserve(vec.size());
            std::transform(std::begin(vec), std::end(vec), std::back_inserter(path), [](std::string const &str) {
                return cyng::to_obis(str);
            });

            return path;
        }

        std::set<cyng::obis> to_obis_set(cyng::vector_t vec) {
            std::set<cyng::obis> r;
            std::transform(vec.begin(), vec.end(), std::inserter(r, r.end()), [](cyng::object const &obj) {
                return cyng::value_cast(obj, cyng::obis{});
            });
            BOOST_ASSERT(vec.size() >= r.size());
            return r;
        }
        void to_decimal(std::ostream &os, cyng::obis const &o) {
            os << std::dec << +o.get_medium() << '-' << +o.get_channel() << ':' << +o.get_indicator() << '.' << +o.get_mode() << '.'
               << +o.get_quantity() << '*' << +o.get_storage();
        }

        std::string to_decimal(cyng::obis const &o) {
            std::ostringstream ss;
            to_decimal(ss, o);
            return ss.str();
        }

    } // namespace obis
} // namespace smf
