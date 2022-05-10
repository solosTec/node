/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_CONFIG_BLEND_H
#define SMF_CONFIG_BLEND_H

#include <cyng/db/session.h>
#include <cyng/store/meta.h>

namespace smf {
    namespace config {

        /**
         * Make a record from a compatible in-memory table persistent
         */
        bool persistent_insert(cyng::meta_sql const &, cyng::db::session, cyng::key_t key, cyng::data_t data, std::uint64_t gen);

    } // namespace config
} // namespace smf

#endif
