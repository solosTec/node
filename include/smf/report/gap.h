/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_GAP_H
#define SMF_GAP_H

#include <smf/mbus/server_id.h>
#include <smf/report/sml_data.h>
#include <smf/report/utility.h>

#include <cyng/db/session.h>
#include <cyng/log/logger.h>

#include <set>

namespace smf {

    void generate_gap(cyng::db::session, cyng::obis profile, std::filesystem::path, cyng::date, std::chrono::hours backtrack);

    namespace gap {
        /**
         * remove only readout
         */
        void clear_data(readout_t &);

        /**
         * opens the report file
         */
        std::ofstream open_report(std::filesystem::path root, std::string file_name);

        /**
         * Generate the gap report
         */
        void generate_report(std::ofstream &, cyng::obis profile, cyng::date const &d, readout_t const &data);

    } // namespace gap

} // namespace smf

#endif
