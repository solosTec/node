/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Sylko Olzscher
 *
 */
#ifndef SMF_DNS_PARSER_H
#define SMF_DNS_PARSER_H

#include <smf/dns/header.h>

#include <cstdint>
#include <string>

namespace smf {
    namespace dns {
        class parser {
          public:
            parser();
        };

        auto parse_name(const uint8_t *pData) noexcept -> std::string;

        // https://github.com/MikhailGorobets/DNS/blob/master/src/dns.cpp
    } // namespace dns
} // namespace smf

#endif
