/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_REPORT_CSV_H
#define SMF_REPORT_CSV_H

#include <smf/mbus/server_id.h>
#include <smf/report/sml_data.h>
#include <smf/report/utility.h>

#include <cyng/db/session.h>
#include <cyng/log/logger.h>

#include <set>

namespace smf {

    void generate_csv(
        cyng::db::session,
        cyng::obis profile,
        std::filesystem::path root,
        std::string prefix,
        cyng::date now,
        std::chrono::hours backtrack);

    namespace csv {
        std::ofstream open_report(std::filesystem::path root, std::string file_name);
        void generate_report(
            cyng::obis profile,
            cyng::date const &d,
            data::data_set_t const &data_set,
            std::filesystem::path root,
            std::string prefix);
    } // namespace csv

} // namespace smf

#endif
