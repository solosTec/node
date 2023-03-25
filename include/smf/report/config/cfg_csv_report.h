/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_STORE_CONFIG_CSV_REPORT_H
#define SMF_STORE_CONFIG_CSV_REPORT_H

#include <smf/report/config/cfg_report.h>

namespace smf {
    namespace cfg {
        class csv_report : public report {
          public:
            csv_report(cyng::param_map_t &&) noexcept;

            /**
             * @return a set with all defined profiles
             */
            // std::set<cyng::obis> get_profiles() const;
        };
    } // namespace cfg
} // namespace smf

#endif
