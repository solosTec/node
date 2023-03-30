/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Sylko Olzscher
 *
 */
#ifndef SMF_DNS_HEADER_H
#define SMF_DNS_HEADER_H

#include <smf/dns/op_code.h>
#include <smf/dns/r_code.h>

#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace smf {
    namespace dns {

        //  forward declaration
        class parser;

        /**
         * @see https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.1
         *
         *                                1  1  1  1  1  1
         *  0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         * |                      ID                       |
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         * |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         * |                    QDCOUNT                    |
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         * |                    ANCOUNT                    |
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         * |                    NSCOUNT                    |
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         * |                    ARCOUNT                    |
         * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         */
        struct header {
            using value_type = std::uint8_t;
            using SIZE = std::integral_constant<std::size_t, 12>;
            //	internal data type
            using data_type = std::array<value_type, SIZE::value>;
            /**
             * size of internal data buffer
             */
            constexpr static std::size_t size() noexcept { return SIZE::value; }
            header(data_type &);

            /**
             * Each message has a unique id
             */
            std::uint16_t get_id() const noexcept;

            /**
             * std::uint8_t recursion_desired : 1;
             * RD
             */
            bool is_recursion_desired() const noexcept;

            /**
             * std::uint8_t truncation : 1;
             * TC
             */
            bool is_truncated() const noexcept;

            // std::uint8_t authoritative : 1;     //  AA

            /**
             * std::uint8_t opcode : 4;            //  Opcode
             *  0 == standard query
             *  1 == inverse of a query
             *  2 == server status request
             *  3 == reserved
             */
            op_code get_opcode() const noexcept;

            /**
             * QR
             * std::uint8_t is_response_code : 1;
             * @return true if message contains a reponse, otherwise false.
             */
            bool is_query() const noexcept;

            /**
             * std::uint8_t responsecode : 4;
             * RCODE
             */
            r_code get_rcode() const noexcept;

            // std::uint8_t checking_disabled : 1;
            // std::uint8_t authenticated_data : 1;
            // std::uint8_t reserved : 1;
            // std::uint8_t recursion_available : 1;

            /**
             * @return an unsigned 16 bit integer specifying the number of entries in the question section.
             */
            std::uint16_t get_qd_count() const noexcept;

            /**
             * @return an unsigned 16 bit integer specifying the number of resource records in the answer section.
             */
            std::uint16_t get_an_count() const noexcept;

            /**
             * @return an unsigned 16 bit integer specifying the number of name server resource records in the authority records
             * section.
             */
            std::uint16_t get_ns_count() const noexcept;

            /**
             * @return an unsigned 16 bit integer specifying the number of resource records in the additional records section.
             */
            std::uint16_t get_ar_count() const noexcept;

            data_type &data_;
        };
        class msg {

            friend class parser;

          public:
            msg();
            msg(msg const &) = default;

            /**
             * readonly access to header data
             */
            header const &get_header() const;

          private:
            header::data_type data_;
            header header_;
            std::vector<std::string> qname_;
        };

    } // namespace dns

    template <typename ch, typename char_traits>
    std::basic_ostream<ch, char_traits> &operator<<(std::basic_ostream<ch, char_traits> &os, dns::header const &header) {
        os << std::hex << std::setfill('0') << std::setw(2) << header.get_id() << '-' << (header.is_query() ? 'Q' : 'R') << std::dec
           << '-' << header.get_opcode() << '-' << header.get_rcode() << "-qd:" << header.get_qd_count()
           << "-an:" << header.get_an_count() << "-ns:" << header.get_ns_count() << "-an:" << header.get_ar_count();
        return os;
    }

    template <typename ch, typename char_traits>
    std::basic_ostream<ch, char_traits> &operator<<(std::basic_ostream<ch, char_traits> &os, dns::msg const &m) {
        //  ToDo:
        os << m.get_header();
        return os;
    }

} // namespace smf

#endif
