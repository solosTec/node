/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_OBIS_PROFILE_H
#define SMF_OBIS_PROFILE_H

#include <cyng/obj/intrinsics/date.h>
#include <cyng/obj/intrinsics/obis.h>

#include <chrono>

namespace smf {
    namespace sml {

        /**
         * @return A vector of all defined load profiles.
         */
        cyng::obis_path_t get_profiles();

        /**
         * @return true if code is a (supported) load profile
         */
        bool is_profile(cyng::obis code);

        /**
         * @param tp timepoint in the specified period of time. Only required for monthly and annual profiles
         * @return interval time for the specified profile
         */
        std::chrono::minutes interval_time(std::chrono::system_clock::time_point, cyng::obis profile);

        /**
         * @return a default backtrack time
         */
        std::chrono::hours backtrack_time(cyng::obis profile);

        /**
         * Computes the largest time point not greater than arg.
         *
         * Same effect as:
         * @code
         * smf::sml::to_time_point(smf::sml::to_index(now, profile), profile);
         * @endcode
         */
        std::chrono::system_clock::time_point floor(std::chrono::system_clock::time_point, cyng::obis profile);

        /**
         * Stick to nearest time point.
         */
        std::chrono::system_clock::time_point round(std::chrono::system_clock::time_point, cyng::obis profile);

        /**
         * Put the length of an interval in a certain grid depended from the profile and guaranties
         * that the interval is not null.
         * int is used based on the following data types from the chrono library:
         * @code
         * using minutes      = duration<int, ratio<60>>;
         * using hours        = duration<int, ratio<3600>>;
         * @endcode
         */
        std::int64_t rasterize_interval(std::int64_t sec, cyng::obis profile);

        /**
         * Calculate next time point to push data for specified profile.
         *
         */
        std::chrono::system_clock::time_point
        next(std::chrono::seconds interval, cyng::obis profile, std::chrono::system_clock::time_point);

        /**
         * Calculate an index of the time stamp with an offset to 1. january 2022.
         */
        std::pair<std::int64_t, bool> to_index(std::chrono::system_clock::time_point, cyng::obis profile);
        std::pair<std::int64_t, bool> to_index(cyng::date, cyng::obis profile);

        /**
         * Each profile has a specific count of entries in a time span. e.g. a 15 minutes report has 96 entries
         * a day.
         * Doesn't work for profiles with a variable time span.
         */
        std::size_t calculate_entry_count(cyng::obis profile, std::chrono::hours);

        /**
         * Calculate the time point of the index
         */
        std::chrono::system_clock::time_point restore_time_point(std::int64_t, cyng::obis profile);

        /**
         * @return the offset date (2022-01-01 00:00:00) as UTC.
         */
        cyng::date get_offset();

        //  2022-01-01 00:00:00 (UTC)
        constexpr static std::chrono::time_point<std::chrono::system_clock> offset_ =
            std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(1640995200));
    } // namespace sml

    /**
     * @return minutes since Unix epoch (00:00:00 UTC on 1 January 1970)
     */
    std::chrono::minutes minutes_since_epoch(std::chrono::system_clock::time_point tp);

    /**
     * std::chrono::day requires C++20
     *
     * @return hours since Unix epoch (00:00:00 UTC on 1 January 1970)
     */
    std::chrono::hours hours_since_epoch(std::chrono::system_clock::time_point);

    /**
     * Calculate the offset time to 2022-01-01 00:00:00
     */
    std::chrono::system_clock::time_point calculate_offset(std::chrono::system_clock::time_point);

} // namespace smf

#endif
