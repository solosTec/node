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
#include <smf/report/utility.h>

#include <cyng/db/session.h>
#include <cyng/log/logger.h>

#include <set>

namespace smf {

    /**
     * Callback for progress of report generation
     */
    using lpex_cb = std::function<void(std::chrono::system_clock::time_point, std::chrono::minutes, std::size_t, std::size_t)>;

    /**
     * Generate a LPEx report of the specified profile
     *
     * @param separated generated for each device individually
     */
    void generate_lpex(
        cyng::db::session,
        cyng::obis profile,
        cyng::obis_path_t filter,
        std::filesystem::path,
        cyng::date now,
        std::chrono::hours backtrack,
        std::string prefix,
        bool print_version,
        bool separated,
        bool debug_mode);

    namespace lpex {

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

        std::size_t collect_report(
            cyng::db::session,
            report_range const &subrr,
            cyng::obis_path_t const &filter,
            std::filesystem::path root,
            std::string prefix,
            bool print_version,
            bool separated,
            bool debug_mode);

        void emit_report(
            std::ofstream &ofs,
            report_range const &subrr,
            srv_id_t srv_id,
            bool debug_mode,
            data::values_t const &,
            std::optional<lpex_customer> const &customer_data);

        void emit_line(std::ostream &, std::vector<std::string> const);

        void emit_data(
            std::ostream &,
            report_range const &subrr,
            srv_id_t srv_id,
            bool debug_mode,
            data::values_t const &,
            std::optional<lpex_customer> const &customer_data);

        void emit_customer_data(std::ostream &os, srv_id_t srv_id, std::optional<lpex_customer> const &customer_data);
        void emit_values(
            std::ostream &os,
            report_range const &subrr,
            srv_id_t srv_id,
            bool debug_mode,
            std::uint64_t first_slot, // first slot with data
            std::map<std::int64_t, sml_data> const &load);

        std::ofstream open_report(std::filesystem::path root, std::string file_name, bool print_version);

        void generate_report(std::ofstream &, cyng::obis profile, cyng::date const &d, data::data_set_t const &data);
        void clear_data(data::data_set_t &data);
        void update_data_set(
            smf::srv_id_t id,
            data::data_set_t &,
            cyng::obis reg,
            std::uint64_t slot,
            std::uint16_t code,
            std::int8_t scaler,
            std::uint8_t unit,
            std::string value,
            std::uint32_t status);

    } // namespace lpex

} // namespace smf

#endif
