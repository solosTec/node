/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_OBIS_PROFILE_H
#define SMF_OBIS_PROFILE_H

#include <cyng/obj/intrinsics/obis.h>

#include <chrono>

namespace smf {
    namespace sml {

        /**
         * @return A vector of all defined load profiles.
         */
        cyng::obis_path_t get_profiles();

        /**
         * @return interval time for the specified profile
         */
        std::chrono::seconds interval_time(cyng::obis profile);

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
         * calculate next time point to push data for specified profile
         */
        std::chrono::system_clock::time_point
        next(std::chrono::seconds interval, cyng::obis profile, std::chrono::system_clock::time_point);

        /**
         * Calculate an index of the time stamp with an offset to 1. january 2022.
         */
        std::int64_t to_index(std::chrono::system_clock::time_point, cyng::obis profile);

        /**
         * Calculate the time point of the index
         */
        std::chrono::system_clock::time_point to_time_point(std::int64_t, cyng::obis profile);

    } // namespace sml

    /**
     * @return minutes since Unix epoch (00:00:00 UTC on 1 January 1970)
     */
    std::chrono::minutes minutes_since_epoch(std::chrono::system_clock::time_point tp);

    /**
     * @return hours since Unix epoch (00:00:00 UTC on 1 January 1970)
     */
    std::chrono::hours hours_since_epoch(std::chrono::system_clock::time_point);

    /**
     * Calculate the offset time to 2022-01-01 00:00:00
     */
    std::chrono::system_clock::time_point calculate_offset(std::chrono::system_clock::time_point);

    //  2022-01-01 00:00:00
    constexpr static std::chrono::time_point<std::chrono::system_clock> offset_ =
        std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(1640991600));
} // namespace smf

#endif
