/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_CONFIG_REPORT_H
#define SMF_STORE_CONFIG_REPORT_H

#include <cyng/obj/intrinsics.h>

#include <chrono>

namespace smf {
    namespace cfg {
        class report {
          public:
            report(cyng::param_map_t &&) noexcept;

            /**
             * @return reference to "db" section
             */
            std::string get_db() const;

            /**
             * @return a set with all defined profiles
             */
            std::set<cyng::obis> get_profiles() const;

            /**
             * "enabled"
             */
            bool is_enabled(cyng::obis) const;

            /**
             * "name"
             */
            std::string get_name(cyng::obis) const;

            /**
             * "path"
             */
            std::string get_path(cyng::obis) const;

            /**
             * "prefix"
             */
            std::string get_prefix(cyng::obis) const;

            /**
             * "backtrack"
             */
            std::chrono::hours get_backtrack(cyng::obis code) const;

          protected:
            cyng::param_map_t get_profile_section() const;
            cyng::param_map_t get_config(cyng::obis) const;

          protected:
            cyng::param_map_t pm_;
        };
    } // namespace cfg
} // namespace smf

#endif
