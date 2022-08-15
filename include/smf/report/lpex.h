/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_LPEX_H
#define SMF_LPEX_H

#include <smf/mbus/server_id.h>
#include <smf/report/sml_data.h>

#include <cyng/db/session.h>
#include <cyng/log/logger.h>

#include <set>

namespace smf {

    void generate_lpex(
        cyng::db::session,
        cyng::obis profile,
        std::filesystem::path,
        std::chrono::hours backtrack,
        std::chrono::system_clock::time_point,
        std::string prefix,
        std::chrono::minutes utc_offset,
        bool print_version);

    namespace lpex {
        /**
         * 1 minute reports
         */
        void generate_report_1_minute(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            std::chrono::minutes utc_offset,
            bool print_version);

        /**
         * Quarter hour reports.
         * Each report can contains a full month over all meters. But since this is a lot
         * of data the report will be split into daily reports.
         * Each line contains all values of *one* register data for this day.
         * So we have multiple lines for the same meter (and customer)
         */
        void generate_report_15_minutes(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            std::chrono::minutes utc_offset,
            bool print_version);

        std::chrono::system_clock::time_point generate_report_15_minutes(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::hours,
            bool print_version);

        /**
         * Hourly reports
         */
        void generate_report_60_minutes(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            std::chrono::minutes utc_offset,
            bool print_version);

        /**
         * Daily reports
         */
        void generate_report_24_hour(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            std::chrono::minutes utc_offset,
            bool print_version);

        /**
         * Monthly reports
         */
        void generate_report_1_month(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            std::chrono::minutes utc_offset,
            bool print_version);

        /**
         * Yearly reports
         */
        void generate_report_1_year(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            std::chrono::minutes utc_offset,
            bool print_version);

        /**
         * Each LPEX file starts with the same header (in german)
         */
        std::vector<std::string> get_header();

        /**
         * Fill up to 2* 96 entries
         */
        std::vector<std::string> fill_up(std::vector<std::string> &&);

        /**
         * Each LPEX file starts with a single line that conatains the version number
         * LPEX V2.0
         */
        std::vector<std::string> get_version();

        void collect_report(
            cyng::db::session,
            cyng::obis profile,
            std::filesystem::path root,
            std::string prefix,
            std::chrono::system_clock::time_point,
            std::chrono::system_clock::time_point,
            srv_id_t,
            bool print_version);

        void emit_report(
            std::filesystem::path root,
            std::string file_name,
            cyng::obis profile,
            srv_id_t srv_id,
            bool print_version,
            std::map<cyng::obis, std::map<std::int64_t, sml_data>> const &data,
            std::optional<lpex_customer> const &customer_data);

        void emit_line(std::ostream &, std::vector<std::string> const);

        void emit_data(
            std::ostream &,
            cyng::obis profile,
            srv_id_t srv_id,
            std::map<cyng::obis, std::map<std::int64_t, sml_data>> const &data,
            std::optional<lpex_customer> const &customer_data);

        void emit_customer_data(std::ostream &os, srv_id_t srv_id, std::optional<lpex_customer> const &customer_data);
        void emit_values(
            std::ostream &os,
            cyng::obis profile,
            srv_id_t srv_id,
            std::chrono::system_clock::time_point,
            std::map<std::int64_t, sml_data> const &load);

    } // namespace lpex

} // namespace smf

#endif
