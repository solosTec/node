/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_REPORT_UTILITY_H
#define SMF_REPORT_UTILITY_H

#include <smf/mbus/server_id.h>
#include <smf/report/sml_data.h>

#include <cyng/db/session.h>
#include <cyng/log/logger.h>

#include <optional>
#include <set>

namespace smf {

    /**
     * @return all meters that have data of the specified profile
     * in this time range.
     */
    [[nodiscard]] std::vector<cyng::buffer_t> select_meters(
        cyng::db::session db,
        cyng::obis profile,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end);

    /**
     * @return all SML measuring data of one readout
     */
    [[nodiscard]] std::map<cyng::obis, sml_data> select_readout_data(cyng::db::session db, boost::uuids::uuid);

    /**
     * @return all distinct registers used in this data set
     */
    [[nodiscard]] std::set<cyng::obis> collect_profiles(std::map<std::uint64_t, std::map<cyng::obis, sml_data>> const &);

    std::string get_filename(std::string prefix, cyng::obis profile, srv_id_t, std::chrono::system_clock::time_point);

    /**
     * @return a short prefix specific for the given profile.
     */
    std::string get_prefix(cyng::obis profile);

    /**
     * Collect data over the specified time range
     */
    std::map<std::uint64_t, std::map<cyng::obis, sml_data>> collect_data_by_timestamp(
        cyng::db::session db,
        cyng::obis profile,
        cyng::buffer_t,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end);

    std::map<cyng::obis, std::map<std::int64_t, sml_data>> collect_data_by_register(
        cyng::db::session db,
        cyng::obis profile,
        cyng::buffer_t,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end);

    /**
     *
     */
    std::optional<lpex_customer> query_customer_data_by_meter(cyng::db::session db, cyng::buffer_t id);

    cyng::meta_store get_store_virtual_sml_readout();
    cyng::meta_sql get_table_virtual_sml_readout();

    cyng::meta_store get_store_virtual_customer();
    cyng::meta_sql get_table_virtual_customer();

} // namespace smf

#endif
