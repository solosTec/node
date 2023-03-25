/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_CONFIG_GAP_H
#define SMF_STORE_CONFIG_GAP_H

#include <cyng/obj/intrinsics.h>

#include <chrono>

namespace smf {
    namespace cfg {
        class gap {
          public:
            gap(cyng::param_map_t &&) noexcept;

            /**
             * @return a set with all defined profiles
             */
            std::set<cyng::obis> get_profiles() const;

            bool is_enabled(cyng::obis) const;
            std::string get_name(cyng::obis) const;
            std::string get_path(cyng::obis) const;
            std::chrono::hours get_backtrack(cyng::obis) const;

          private:
            cyng::param_map_t pm_;
        };
    } // namespace cfg
} // namespace smf

#endif
