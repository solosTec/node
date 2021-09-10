/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_OBIS_CONV_H
#define SMF_OBIS_CONV_H

#include <smf/obis/defs.h>
#include <vector>

namespace smf {
    namespace obis {

        std::vector<std::string> to_str_vector(cyng::obis_path_t const &path, bool translate);

        cyng::obis_path_t to_obis_path(std::vector<std::string> const &vec);

    } // namespace obis
} // namespace smf

#endif
