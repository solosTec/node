/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_SEGW_PROFILES_H
#define NODE_IPT_SEGW_PROFILES_H

#include <smf/sml/intrinsics/obis.h>
#include <chrono>

namespace node
{
    namespace sml
    {

        /**
         * Put the length of an interval in a certain grid depended from the profile.
         */
        std::uint32_t rasterize_interval(std::uint32_t sec, obis profile);

        /**
         * calculate next time point to push data for specified profile
         */
        std::chrono::system_clock::time_point next(std::chrono::seconds interval, obis profile);
    }
}

#endif
