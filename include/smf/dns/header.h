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
        /**
         * @see https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.1
         *
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         * | ID |
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         * |QR| Opcode |AA|TC|RD|RA| Z | RCODE |
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         * | QDCOUNT |
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         * | ANCOUNT |
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         * | NSCOUNT |
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         * | ARCOUNT |
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         */
        struct header {
            std::uint16_t id;
            union {
                struct {
                    std::uint8_t recursion_desired : 1; //  RD
                    std::uint8_t truncation : 1;        //  TC
                    std::uint8_t authoritative : 1;     //  AA
                    std::uint8_t opcode : 4;            //  Opcode
                    std::uint8_t is_response_code : 1;  //  QR

                    std::uint8_t responsecode : 4; //  RCODE
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

        // Constant sized fields of query structure
        struct question {
            std::uint16_t qtype = 1;
            std::uint16_t qclass = 1;
        };

// Constant sized fields of the resource record structure
#pragma pack(push, 1)
        struct answer {
            std::uint16_t type;
            std::uint16_t _class;
            unsigned int ttl;
            std::uint16_t data_len;
        };
#pragma pack(pop)
        // Pointers to resource record contents
        // struct res_record {
        //    unsigned char *name;
        //    struct r_data *resource;
        //    unsigned char *rdata;
        //};

        // Structure of a Query
        struct query {
            unsigned char *name;
            struct question *ques;
        };

        // Types of DNS resource records
        enum resource_type : std::uint16_t {
            // #define T_A 1     // Ipv4 address
            // #define T_NS 2    // Nameserver
            // #define T_CNAME 5 // canonical name
            // #define T_SOA 6   // start of authority zone
            // #define T_PTR 12  // domain name pointer
            // #define T_MX 15   // Mail server
            // a host address
            RES_A = 1,      // an authoritative name server
            RES_NS = 2,     // a mail destination (Obsolete - use MX)
            RES_MD = 3,     // a mail forwarder (Obsolete - use MX)
            RES_MF = 4,     // the canonical name for an alias
            RES_CNAME = 5,  // marks the start of a zone of authority
            RES_SOA = 6,    // a mailbox domain name (EXPERIMENTAL)
            RES_MB = 7,     // a mail group member (EXPERIMENTAL)
            RES_MG = 8,     // a mail rename domain name (EXPERIMENTAL)
            RES_MR = 9,     // a null RR (EXPERIMENTAL)
            RES_NULL = 10,  // a well known service description
            RES_WKS = 11,   // a domain name pointer
            RES_PTR = 12,   // host information
            RES_HINFO = 13, // mailbox or mail list information
            RES_MINFO = 14, // mail exchange
            RES_MX = 15,    // text strings
            RES_TXT = 16,   // IPv6 address
            RES_AAAA = 28,  // service record specifies
            RES_SRV = 33,   // naming authority pointer
            RES_NAPTR = 35,
            RES_A6 = 0x0026,
            RES_OPT = 0x0029,
            RES_ANY = 0x00ff
        }

    } // namespace dns
} // namespace smf

#endif
