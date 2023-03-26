/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_GEN_FEED_H
#define SMF_STORE_GEN_FEED_H

#include <cyng/obj/intrinsics.h>

#include <chrono>

namespace smf {
    namespace cfg {
        /**
         * @return default configuration for feed reports
         */
        cyng::tuple_t gen_feed(std::filesystem::path cwd);

        cyng::prop_t create_feed_spec(cyng::obis profile, std::filesystem::path cwd, bool enabled, std::chrono::hours backtrack);
    } // namespace cfg
} // namespace smf

#endif
