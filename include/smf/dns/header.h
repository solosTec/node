/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_DNS_HEADER_H
#define SMF_DNS_HEADER_H

#include <cstdint>
#include <string>

namespace smf {
    namespace dns {
        struct header {
            std::uint16_t id;
            union {
                struct {
                    std::uint8_t recursion_desired : 1;
                    std::uint8_t truncation : 1;
                    std::uint8_t authoritative : 1;
                    std::uint8_t opcode : 4;
                    std::uint8_t is_response_code : 1;

                    std::uint8_t responsecode : 4;
                    std::uint8_t checking_disabled : 1;
                    std::uint8_t authenticated_data : 1;
                    std::uint8_t reserved : 1;
                    std::uint8_t recursion_available : 1;
                };
                struct {
                    std::uint16_t flags;
                };
                struct {
                    std::uint8_t flags1;
                    std::uint8_t flags2;
                };
            };
            std::uint16_t count_question;
            std::uint16_t count_answer;
            std::uint16_t count_authority;
            std::uint16_t count_additional;
        };
    } // namespace dns
} // namespace smf

#endif
