/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/obis/conv.h>
#include <smf/obis/db.h>

#include <cyng/obj/util.hpp>
#include <cyng/parse/buffer.h>

#include <algorithm>
#include <iterator>

#include <boost/assert.hpp>

namespace smf {
    namespace obis {

        std::vector<std::string> to_str_vector(cyng::obis_path_t const &path, bool translate) {
            std::vector<std::string> vec;
            std::transform(path.begin(), path.end(), std::back_inserter(vec), [&](cyng::obis code) {
                return (translate) ? get_name(code) : cyng::to_str(code);
            });
            return vec;
        }

        cyng::obis_path_t to_obis_path(std::vector<std::string> const &vec) {
            cyng::obis_path_t path;
            path.reserve(vec.size());
            std::transform(std::begin(vec), std::end(vec), std::back_inserter(path), [](std::string const &str) {
                return cyng::make_obis(cyng::hex_to_buffer(str));
            });

            return path;
        }

    } // namespace obis
} // namespace smf
