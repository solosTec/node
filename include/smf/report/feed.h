/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_FEED_H
#define SMF_FEED_H

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
    void generate_feed(
        cyng::db::session,
        cyng::obis profile,
        // cyng::obis_path_t filter,
        std::filesystem::path root,
        std::string prefix,
        cyng::date now,
        std::chrono::hours backtrack,
        bool print_version,
        bool debug_mode,
        bool customer,
        std::size_t shift_factor);

    namespace feed {

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

        void emit_line(std::ostream &, std::vector<std::string> const);

        void emit_customer_data(std::ostream &os, std::string srv_id, std::optional<lpex_customer> const &customer_data);

        /**
         * opens the report file
         */
        std::ofstream open_report(std::filesystem::path root, std::string file_name, bool print_version);

        /**
         * Generate the LPex report
         */
        void generate_report(
            cyng::db::session db,
            std::ofstream &,
            cyng::obis profile,
            cyng::date const &start,
            cyng::date const &end,
            data::data_set_t const &data,
            bool debug_mode,
            bool customer,
            std::size_t shift_factor);

        /**
         * We need two time units of lead.
         */
        cyng::date calculate_start_time(
            cyng::date const &now,
            std::chrono::hours const &backtrack,
            cyng::obis const &profile,
            std::size_t shift_factor);

        /**
         * Calculate value feed/adcance
         */
        std::string calculate_advance(sml_data const &, sml_data const &);

        /**
         * Check if obis code is convertible and then return true with the profile code
         */
        std::pair<cyng::obis, bool> convert_obis(cyng::obis const &);

    } // namespace feed

} // namespace smf

#endif
