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

    void generate_gap(
        cyng::db::session,
        cyng::obis profile,
        std::filesystem::path,
        std::chrono::hours backtrack,
        std::chrono::system_clock::time_point);

    namespace gap {

        /**
         * 1 minute reports
         */
        void generate_report_1_minute(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::chrono::system_clock::time_point start,
            std::chrono::system_clock::time_point end);

        /**
         * Quarter hour reports.
         * Each report can contains a full month over all meters. But since this is a lot
         * of data the report will be split into daily reports.
         * Each line contains all values of *one* register data for this day.
         * So we have multiple lines for the same meter (and customer)
         */
        void generate_report_15_minutes(cyng::db::session, report_range const &rr, std::filesystem::path root);

        gap::readout_t generate_report_15_minutes(
            cyng::db::session db,
            gap::readout_t const &initial_data,
            report_range const &rr,
            std::filesystem::path root);

        /**
         * Hourly reports
         */
        void generate_report_60_minutes(cyng::db::session, report_range const &rr, std::filesystem::path root);

        gap::readout_t generate_report_60_minutes(
            cyng::db::session db,
            gap::readout_t const &initial_data,
            report_range const &rr,
            std::filesystem::path root);

        /**
         * Daily reports
         */
        void generate_report_24_hour(cyng::db::session, report_range const &rr, std::filesystem::path root);

        gap::readout_t generate_report_24_hour(
            cyng::db::session db,
            gap::readout_t const &initial_data,
            report_range const &rr,
            std::filesystem::path root);

        /**
         * Monthly reports
         */
        void generate_report_1_month(cyng::db::session, report_range const &rr, std::filesystem::path root);

        /**
         * Yearly reports
         */
        void generate_report_1_year(cyng::db::session, report_range const &rr, std::filesystem::path root);

        gap::readout_t
        collect_report(cyng::db::session, gap::readout_t const &initial_data, report_range const &rr, std::filesystem::path root);

        void
        emit_data(std::ostream &, report_range const &subrr, srv_id_t srv_id, std::int64_t start_slot, gap::slot_date_t const &);

    } // namespace gap

} // namespace smf

#endif
