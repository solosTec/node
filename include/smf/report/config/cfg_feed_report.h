/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_CONFIG_FEED_REPORT_H
#define SMF_STORE_CONFIG_FEED_REPORT_H

#include <smf/report/config/cfg_report.h>

namespace smf {
    namespace cfg {
        class feed_report : public report {
          public:
            feed_report(cyng::param_map_t &&) noexcept;

            /**
             * "debug"
             */
            bool is_debug_mode() const;

            /**
             * "print.version"
             */
            bool is_print_version() const;

            /**
             * "filter"
             */
            cyng::obis_path_t get_filter() const;

            /**
             * "add.customer.data"
             */
            bool add_customer_data(cyng::obis code) const;
        };
    } // namespace cfg
} // namespace smf

#endif
