/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_REPORT_UTILITY_H
#define SMF_REPORT_UTILITY_H

#include <smf/obis/db.h>
#include <smf/report/sml_data.h>

#include <cyng/db/session.h>
#include <cyng/log/logger.h>
#include <cyng/obj/intrinsics/date.h>

#include <iostream>
#include <optional>
#include <ostream>
#include <set>
#include <utility>

namespace smf {

    /**
     * data structure to hold a complete profile readout
     */
    namespace data {

        //
        //  slot -> data
        //
        // using readout_t = std::map<std::int64_t, sml_data>;

        //
        //  register -> slot -> data
        //
        // using values_t = std::map<cyng::obis, readout_t>;

        //
        //  data set of the full period
        //  meter -> register -> slot -> data
        //
        // using data_set_t = std::map<smf::srv_id_t, values_t>;

        typename readout_t::value_type make_readout(
            std::int64_t slot,
            std::uint16_t code,
            std::int8_t scaler,
            std::uint8_t unit,
            std::string reading,
            std::uint32_t status);

        typename values_t::value_type make_value(cyng::obis, readout_t::value_type);
        typename data_set_t::value_type make_set(smf::srv_id_t, values_t::value_type);

        /**
         * remove only readout data
         */
        void clear(data_set_t &data);

        /**
         * merge incoming data into data set
         */
        void update(
            data_set_t &,
            smf::srv_id_t id,
            cyng::obis reg,
            std::uint64_t slot,
            std::uint16_t code,
            std::int8_t scaler,
            std::uint8_t unit,
            std::string value,
            std::uint32_t status);

    } // namespace data

    namespace gap {
        using slot_date_t = std::map<std::uint64_t, cyng::date>; // slot => actTime
        using readout_t = std::map<srv_id_t, slot_date_t>;       // meter => slot

        typename slot_date_t::value_type make_slot(std::uint64_t, cyng::date);
        typename readout_t::value_type make_readout(srv_id_t, slot_date_t::value_type);

    } // namespace gap

    /**
     * @param status comes from table "TSMLReadout"
     * @return all SML measuring data of one readout
     */
    [[nodiscard]] std::map<cyng::obis, sml_data>
    select_readout_data(cyng::db::session db, boost::uuids::uuid, std::uint32_t status);

    using cb_loop_readout = std::function<bool(
        bool,
        boost::uuids::uuid,
        smf::srv_id_t,
        cyng::date,
        std::uint32_t,
        cyng::obis,
        std::string,
        std::uint16_t,
        std::int8_t,
        mbus::unit)>;

    /**
     * loop over result set
     */
    std::size_t loop_readout_data(cyng::db::session db, cyng::obis profile, cyng::date start, cb_loop_readout);

    using cb_loop_meta = std::function<bool(smf::srv_id_t, cyng::date)>;

    /**
     * loop over meta data
     */
    std::size_t loop_readout(cyng::db::session db, cyng::obis profile, cyng::date start, cb_loop_meta);

    /**
     * @return true if reg is an element of the filter or filter is empty
     */
    bool has_passed(cyng::obis reg, cyng::obis_path_t const &filter);

    /**
     * @return all distinct registers used in this data set
     */
    [[nodiscard]] std::set<cyng::obis> collect_profiles(std::map<std::uint64_t, std::map<cyng::obis, sml_data>> const &);

    std::string get_filename(std::string prefix, cyng::obis profile, srv_id_t, cyng::date const &);
    std::string get_filename(std::string prefix, cyng::obis profile, cyng::date const &);

    /**
     * get customer data from database
     */
    std::optional<lpex_customer> query_customer_data_by_meter(cyng::db::session db, cyng::buffer_t id);

    cyng::meta_store get_store_virtual_sml_readout();
    cyng::meta_sql get_table_virtual_sml_readout();

    cyng::meta_store get_store_virtual_customer();
    cyng::meta_sql get_table_virtual_customer();

    /**
     * remove outdated records of the specified profile
     *
     * @param limit max number of records to delete
     */
    std::size_t cleanup(cyng::db::session db, cyng::obis profile, cyng::date);

    /**
     * calls the vacuum command
     */
    void vacuum(cyng::db::session db);

    /**
     * simple dump of all collected data
     */
    void dump_readout(cyng::db::session db, cyng::date, std::chrono::hours, std::ostream &os = std::cout);

    /**
     * create path if required
     * @return false if path doesn't exist and cannot be created.
     */
    bool sanitize_path(std::string path);

} // namespace smf

#endif
