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

#include <cyng/db/session.h>
#include <cyng/log/logger.h>

#include <set>

namespace smf {

    void generate_csv(
        cyng::db::session,
        cyng::obis profile,
        std::filesystem::path,
        std::chrono::hours backtrack,
        std::chrono::system_clock::time_point,
        std::string prefix);

    namespace csv {
        /**
         * 1 minute reports
         */
        void generate_report_1_minute(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point);

        std::chrono::system_clock::time_point generate_report_1_minute(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::hours);

        /**
         * Quarter hour reports
         */
        void generate_report_15_minutes(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point);

        std::chrono::system_clock::time_point generate_report_15_minutes(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::hours);

        /**
         * Hourly reports
         */
        void generate_report_60_minutes(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point);

        std::chrono::system_clock::time_point generate_report_60_minutes(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::hours);

        /**
         * Daily reports
         */
        void generate_report_24_hour(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point);

        std::chrono::system_clock::time_point generate_report_24_hour(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::hours);

        /**
         * Monthly reports
         */
        void generate_report_1_month(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point);

        /**
         * Yearly reports
         */
        void generate_report_1_year(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point);

        void collect_report(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            srv_id_t);

        void emit_report(
            std::filesystem::path root,
            std::string file_name,
            cyng::obis profile,
            std::set<cyng::obis> regs,
            std::map<std::uint64_t, std::map<cyng::obis, sml_data>> const &data);

    } // namespace csv

} // namespace smf

#endif
