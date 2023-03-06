/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_CONFIG_PROTOCOLS_H
#define SMF_CONFIG_PROTOCOLS_H

#include <string>

namespace smf {
    namespace config {
        enum class protocol { ANY, RAW, TCP, IPT, IEC, WIRED_MBUS, WIRELESS_MBUS, HDLC, SML, DLMS, COSEM, HAYESAT };

        /*
         * @return the protocol name
         */
        std::string get_name(protocol);

    } // namespace config
} // namespace smf

#endif
