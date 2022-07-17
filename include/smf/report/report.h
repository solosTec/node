/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_REPORT_H
#define SMF_REPORT_H

#include <smf/mbus/server_id.h>
#include <smf/report/sml_data.h>

#include <cyng/db/session.h>
#include <cyng/log/logger.h>

#include <set>

namespace smf {

    void generate_report(
        cyng::db::session,
        cyng::obis profile,
        std::filesystem::path,
        std::chrono::hours backtrack,
        std::chrono::system_clock::time_point);
    void generate_report_1_minute(cyng::db::session, std::chrono::system_clock::time_point, std::chrono::system_clock::time_point);

    void
        generate_report_15_minutes(cyng::db::session, std::chrono::system_clock::time_point, std::chrono::system_clock::time_point);
    std::chrono::system_clock::time_point
        generate_report_15_minutes(cyng::db::session, std::chrono::system_clock::time_point, std::chrono::hours);
    void generate_report_15_minutes(
        cyng::db::session,
        std::chrono::system_clock::time_point,
        std::chrono::system_clock::time_point,
        srv_id_t);

    void
        generate_report_60_minutes(cyng::db::session, std::chrono::system_clock::time_point, std::chrono::system_clock::time_point);
    void generate_report_24_hour(cyng::db::session, std::chrono::system_clock::time_point, std::chrono::system_clock::time_point);
    void generate_report_1_month(cyng::db::session, std::chrono::system_clock::time_point, std::chrono::system_clock::time_point);
    void generate_report_1_year(cyng::db::session, std::chrono::system_clock::time_point, std::chrono::system_clock::time_point);

    [[nodiscard]] std::vector<cyng::buffer_t> select_meters(
        cyng::db::session db,
        cyng::obis profile,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end);

    [[nodiscard]] std::map<cyng::obis, sml_data> select_readout_data(cyng::db::session db, boost::uuids::uuid);

    /**
     * @return all distinct registers used in this data set
     */
    [[nodiscard]] std::set<cyng::obis> collect_profiles(std::map<std::uint64_t, std::map<cyng::obis, sml_data>> const &);

    std::string get_filename(cyng::obis profile, srv_id_t, std::chrono::system_clock::time_point);
    std::string get_prefix(cyng::obis profile);

    void emit_report(
        std::string,
        cyng::obis profile,
        std::set<cyng::obis>,
        std::map<std::uint64_t, std::map<cyng::obis, sml_data>> const &);

} // namespace smf

#endif
