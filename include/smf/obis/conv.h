/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_OBIS_CONV_H
#define SMF_OBIS_CONV_H

#include <smf/obis/defs.h>

#include <cyng/obj/intrinsics/container.h>

#include <iomanip>
#include <set>
#include <sstream>
#include <vector>

namespace smf {
    namespace obis {

        std::vector<std::string> to_str_vector(cyng::obis_path_t const &path, bool translate);
        std::string to_string(cyng::obis_path_t const &path, bool translate, char sep);

        cyng::obis_path_t to_obis_path(std::vector<std::string> const &vec);

        /**
         * Expects a vector of obis objects
         */
        std::set<cyng::obis> to_obis_set(cyng::vector_t vec);

        void to_decimal(std::ostream &os, cyng::obis const &o);
        std::string to_decimal(cyng::obis const &o);

    } // namespace obis
} // namespace smf

#endif
