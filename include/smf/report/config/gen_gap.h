/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_GEN_GAP_H
#define SMF_STORE_GEN_GAP_H

#include <cyng/obj/intrinsics.h>

#include <chrono>

namespace smf {
    namespace cfg {
        /**
         * @return default configuration for gap reports
         */
        cyng::tuple_t gen_gap(std::filesystem::path cwd);
        cyng::prop_t create_gap_spec(cyng::obis profile, std::filesystem::path const &cwd, std::chrono::hours hours, bool enabled);

    } // namespace cfg
} // namespace smf

#endif
