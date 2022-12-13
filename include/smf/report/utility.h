/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_REPORT_UTILITY_H
#define SMF_REPORT_UTILITY_H

#include <smf/mbus/server_id.h>
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
        //  meter -> register -> slot -> data
        //

        using readout_t = std::map<std::int64_t, sml_data>;   // timepoint -> readout
        using values_t = std::map<cyng::obis, readout_t>;     // register -> readout_t
        using profile_t = std::map<cyng::buffer_t, values_t>; // meter -> value_t

        //
        //  data set of the full period
        //
        using data_set_t = std::map<smf::srv_id_t, values_t>; // meter -> value_t

        typename readout_t::value_type make_readout(
            std::int64_t slot,
            std::uint16_t code,
            std::int8_t scaler,
            std::uint8_t unit,
            std::string reading,
            std::uint32_t status);

        typename values_t::value_type make_value(cyng::obis, readout_t::value_type);
        typename profile_t::value_type make_profile(cyng::buffer_t, values_t::value_type);
        typename data_set_t::value_type make_set(smf::srv_id_t, values_t::value_type);

    } // namespace data

    namespace gap {
        using slot_date_t = std::map<std::uint64_t, std::chrono::system_clock::time_point>; //  slot => actTime
        using readout_t = std::map<cyng::buffer_t, slot_date_t>;                            // meter => slot

        typename slot_date_t::value_type make_slot(std::uint64_t, std::chrono::system_clock::time_point);
        typename readout_t::value_type make_readout(cyng::buffer_t, slot_date_t::value_type);

    } // namespace gap

    /**
     * Helper class to store report specific data
     */
    class report_range {
      public:
        report_range(cyng::obis profile, cyng::date const &start, cyng::date const &end);

        template <typename R, typename P>
        report_range(cyng::obis profile, cyng::date const &start, std::chrono::duration<R, P> d)
            : report_range(profile, start, start.add(d)) {}

        report_range(report_range const &) = default;
        report_range(report_range &&) = default;

        cyng::date const &get_start() const noexcept;
        cyng::date const &get_end() const noexcept;

        /**
         * both values as pair
         */
        std::pair<cyng::date, cyng::date> get_range() const noexcept;
        std::pair<std::uint64_t, std::uint64_t> get_slots() const noexcept;

        cyng::obis const &get_profile() const noexcept;

        /**
         * timespan of intervall
         */
        std::chrono::hours get_span() const;

        template <typename CharT, typename Traits>
        friend std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &os, report_range const &rr) {

            os << obis::get_name(rr.get_profile()) << ":";
            // os << cyng::to_string(rr.get_profile()) << ": ";

            cyng::as_string(os, rr.get_start(), "%Y-%m-%d %H:%M:%S");
            os << " ==" << rr.get_end().sub<std::chrono::hours>(rr.get_start()).count() << "h=> ";
            cyng::as_string(os, rr.get_end(), "%Y-%m-%d %H:%M:%S");

            return os;
        }

        /**
         * Calculate the maximal count of readouts for this profile in this timespan.
         */
        std::size_t max_readout_count() const;

      private:
        cyng::obis profile_;
        cyng::date start_;
        cyng::date end_;
    };

    /**
     * @return start time for a reporting period.
     */
    // cyng::date calculate_start_time(cyng::date now, cyng::obis profile);

    /**
     * @return all meters that have data of the specified profile
     * in this time range.
     */
    [[nodiscard, deprecated]] std::vector<cyng::buffer_t> select_meters(cyng::db::session db, report_range const &rr);

    /**
     * @param status comes from table "TSMLReadout"
     * @return all SML measuring data of one readout
     */
    [[nodiscard]] std::map<cyng::obis, sml_data>
    select_readout_data(cyng::db::session db, boost::uuids::uuid, std::uint32_t status);

    using loop_cb = std::function<bool(
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
    std::size_t loop_readout_data(cyng::db::session db, cyng::obis profile, cyng::date start, loop_cb);

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
    std::string get_filename(
        std::string prefix,
        cyng::obis profile,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end);

    /**
     * @return a short prefix specific for the given profile.
     */
    std::string get_prefix(cyng::obis profile);

    /**
     * Collect data over the specified time range
     */
    std::map<std::uint64_t, std::map<cyng::obis, sml_data>>
    collect_data_by_timestamp(cyng::db::session db, report_range const &subrr, cyng::buffer_t id);

    data::profile_t collect_data_by_time_range(cyng::db::session db, cyng::obis_path_t const &filter, report_range const &);

    gap::readout_t
    collect_readouts_by_time_range(cyng::db::session db, gap::readout_t const &initial_data, report_range const &subrr);

    [[deprecated]] std::map<cyng::obis, std::map<std::int64_t, sml_data>> collect_data_by_register(
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

    /**
     * remove outdated records of the specified profile
     *
     * @param limit max number of records to delete
     */
    std::size_t cleanup(cyng::db::session db, cyng::obis profile, std::chrono::system_clock::time_point, std::size_t limit);

    void update_data(
        data::profile_t &data,
        cyng::buffer_t id,
        cyng::obis reg,
        std::int64_t slot,
        std::uint16_t code,
        std::int8_t scaler,
        std::uint8_t unit,
        std::string const &reading,
        std::uint32_t status);

    void dump_readout(cyng::db::session db, cyng::date, std::chrono::hours);

} // namespace smf

#endif
