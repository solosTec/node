/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_CONFIG_LPEX_REPORT_H
#define SMF_STORE_CONFIG_LPEX_REPORT_H

#include <smf/report/config/cfg_report.h>

namespace smf {
    namespace cfg {
        class lpex_report : public report {
          public:
            lpex_report(cyng::param_map_t &&) noexcept;

            /**
             * @return a set with all defined profiles
             */
            // std::set<cyng::obis> get_profiles() const;

            /**
             * "debug"
             */
            bool is_debug_mode() const;

            /**
             * "print.version"
             */
            bool is_print_version() const;

            /**
             * "add.customer.data"
             */
            bool add_customer_data(cyng::obis code) const;

            /**
             * "filter"
             */
            cyng::obis_path_t get_filter() const;

          private:
            cyng::param_map_t pm_;
        };
    } // namespace cfg
} // namespace smf

#endif
