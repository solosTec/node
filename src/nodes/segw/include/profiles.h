/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_SEGW_PROFILES_H
#define SMF_SEGW_PROFILES_H

#include <cyng/obj/intrinsics/obis.h>

#include <chrono>

namespace smf {
    /**
     * Put the length of an interval in a certain grid depended from the profile.
     */
    std::uint32_t rasterize_interval(std::uint32_t sec, cyng::obis profile);

    /**
     * calculate next time point to push data for specified profile
     */
    std::chrono::system_clock::time_point
    next(std::chrono::seconds interval, cyng::obis profile, std::chrono::system_clock::time_point);

    /**
     * @return minutes since Unix epoch (00:00:00 UTC on 1 January 1970)
     */
    std::chrono::minutes minutes_since_epoch(std::chrono::system_clock::time_point tp);

    /**
     * @return hours since Unix epoch (00:00:00 UTC on 1 January 1970)
     */
    std::chrono::hours hours_since_epoch(std::chrono::system_clock::time_point);

} // namespace smf
#endif
