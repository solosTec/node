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
#include <cyng/sys/clock.h>

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
        using readout_t = std::map<std::int64_t, sml_data>;   //  timepoint -> readout
        using values_t = std::map<cyng::obis, readout_t>;     // register -> readout_t
        using profile_t = std::map<cyng::buffer_t, values_t>; // meter -> value_t

        typename readout_t::value_type make_readout(
            std::int64_t,
            std::uint16_t code,
            std::int8_t scaler,
            std::uint8_t unit,
            std::string reading,
            std::uint32_t status);

        typename values_t::value_type make_value(cyng::obis, readout_t::value_type);
        typename profile_t::value_type make_profile(cyng::buffer_t, values_t::value_type);

    } // namespace data

    namespace gap {
        using slot_date_t = std::map<std::uint64_t, std::chrono::system_clock::time_point>; //  slot => actTime
        using readout_t = std::map<cyng::buffer_t, slot_date_t>;                            // meter => slot

        typename slot_date_t::value_type make_slot(std::uint64_t, std::chrono::system_clock::time_point);
        typename readout_t::value_type make_readout(cyng::buffer_t, slot_date_t::value_type);

    } // namespace gap

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
     * @param status comes from table "TSMLReadout"
     * @return all SML measuring data of one readout
     */
    [[nodiscard]] std::map<cyng::obis, sml_data>
    select_readout_data(cyng::db::session db, boost::uuids::uuid, std::uint32_t status);

    /**
     * @return all distinct registers used in this data set
     */
    [[nodiscard]] std::set<cyng::obis> collect_profiles(std::map<std::uint64_t, std::map<cyng::obis, sml_data>> const &);

    std::string get_filename(std::string prefix, cyng::obis profile, srv_id_t, std::chrono::system_clock::time_point);
    std::string get_filename(std::string prefix, cyng::obis profile, std::chrono::system_clock::time_point);
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
    std::map<std::uint64_t, std::map<cyng::obis, sml_data>> collect_data_by_timestamp(
        cyng::db::session db,
        cyng::obis profile,
        cyng::buffer_t,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end);

    data::profile_t collect_data_by_time_range(
        cyng::db::session db,
        cyng::obis profile,
        cyng::obis_path_t const &filter,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end);

    gap::readout_t collect_readouts_by_time_range(
        cyng::db::session db,
        gap::readout_t const &initial_data,
        cyng::obis profile,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end);

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

    /**
     * Helper class to calculate time zone offset.
     * This is required since date library is not available prior to C++20
     */
    template <typename R, typename P> class tz_offset {
      public:
        using this_type = tz_offset<R, P>;

        tz_offset()
            : tp_()
            , diff_() {}

        tz_offset(std::chrono::system_clock::time_point tp, std::chrono::duration<R, P> diff)
            : tp_(tp)
            , diff_(diff) {}

        tz_offset(tz_offset const &) = default;
        tz_offset(tz_offset &&) = default;

        tz_offset &operator=(std::chrono::system_clock::time_point tp) {
            tp_ = tp;
            return *this;
        }

        tz_offset &operator=(std::chrono::duration<R, P> const &diff) {
            diff_ = diff;
            return *this;
        }

        /** @brief conversion operator
         *
         * @return specified timepoint minus offset (localtime)
         */
        operator std::chrono::system_clock::time_point() const { return utc_time(); }
        operator std::chrono::duration<R, P>() const { return deviation(); }

        /**
         * @return specified timepoint (unmodified).
         */
        std::chrono::system_clock::time_point local_time() const { return tp_; }

        /**
         * @return specified timepoint minus offset.
         */
        std::chrono::system_clock::time_point utc_time() const { return tp_ + diff_; }

        /**
         * @return deviation of local tiem from UTC.
         */
        std::chrono::duration<R, P> deviation() const { return diff_; }

        /**
         * output formatter
         */
        template <typename CharT, typename Traits>
        friend std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &os, this_type const &adj) {
            os << cyng::sys::to_string(adj.local_time(), "%F %T%z ") << adj.deviation() << " "
               << cyng::sys::to_string(adj.utc_time(), "%F %T%z UTC");
            return os;
        }

      private:
        std::chrono::system_clock::time_point tp_;
        std::chrono::duration<R, P> diff_;
    };

    //
    //  type deduction support (the old way)
    //
    template <typename T> struct tz_offset_t {
        using type = void;
    };

    template <typename R, typename P> struct tz_offset_t<std::chrono::duration<R, P>> {
        using type = tz_offset<R, P>;
    };

    /**
     * Factory function for tz_offset
     */
    inline decltype(auto) make_tz_offset(std::chrono::system_clock::time_point tp) {
        return tz_offset(tp, cyng::sys::delta_utc(tp));
    }

} // namespace smf

#endif
