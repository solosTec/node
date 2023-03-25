/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_CONFIG_CLEANUP_H
#define SMF_STORE_CONFIG_CLEANUP_H

#include <cyng/obj/intrinsics.h>

#include <chrono>

namespace smf {
    namespace cfg {
        class cleanup {
          public:
            cleanup(cyng::param_map_t &&) noexcept;

            /**
             * @return a set with all defined profiles
             */
            std::set<cyng::obis> get_profiles() const;

            std::string get_name(cyng::obis) const;
            bool is_enabled(cyng::obis) const;
            std::chrono::hours get_max_age(cyng::obis) const;

          private:
            cyng::param_map_t pm_;
            // cyng::reader<cyng::param_map_t> reader_;
        };
    } // namespace cfg
} // namespace smf

#endif
