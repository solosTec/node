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
        bool
        persistent_insert(cyng::meta_sql const &, cyng::db::session, cyng::key_t key, cyng::data_t const &data, std::uint64_t gen);
        bool
        persistent_update(cyng::meta_sql const &, cyng::db::session, cyng::key_t key, cyng::attr_t const &attr, std::uint64_t gen);
        bool persistent_remove(cyng::meta_sql const &, cyng::db::session, cyng::key_t key);
        bool persistent_clear(cyng::meta_sql const &, cyng::db::session);

    } // namespace config
} // namespace smf

#endif
