/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_CLUSTER_FEATURES_H
#define SMF_CLUSTER_FEATURES_H

#include <cstdint>

namespace smf {

    enum features : std::uint32_t {

        //  no special features
        NO_FEATURES = 0,

        //	able to store and retrieve configuration data
        CONFIG_MANAGER = (1 << 0), // 0b0000000000000001

        //	able to provide and apply configuration data
        CONFIG_PROVIDER = (1 << 1), // 0b0000000000000010

    };

} // namespace smf

#endif
