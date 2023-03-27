/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_SERIALIZER_H
#define SMF_IPT_SERIALIZER_H

#include <smf/ipt.h>
#include <smf/ipt/codes.h>
#include <smf/ipt/header.h>
#include <smf/ipt/scramble_key.h>
#include <smf/ipt/scrambler.hpp>

#include <cyng/obj/buffer_cast.hpp>

#include <deque>

#include <boost/asio.hpp>

namespace smf {

    namespace ipt {

        class serializer {
          public:
            using scrambler_t = scrambler<char, scramble_key::SIZE::value>;
            using seq_generator = details::circular_counter<std::uint8_t, 1, 0xff>;

          public:
            serializer(scramble_key const &);

            [[nodiscard]] cyng::buffer_t req_login_public(std::string const &name, std::string const &pwd);

            /**
             * set new scramble key
             */
            [[nodiscard]] cyng::buffer_t
            req_login_scrambled(std::string const &name, std::string const &pwd, scramble_key const sk);

            [[nodiscard]] cyng::buffer_t res_login_public(response_t res, std::uint16_t watchdog, std::string redirect);

            [[nodiscard]] cyng::buffer_t res_login_scrambled(response_t res, std::uint16_t watchdog, std::string redirect);

            [[nodiscard]] cyng::buffer_t req_watchdog(sequence_t seq);

            /**
             * @seq 0 is response without request
             */
            [[nodiscard]] cyng::buffer_t res_watchdog(sequence_t seq);

            [[nodiscard]] std::pair<cyng::buffer_t, sequence_t> req_open_push_channel(
                std::string,    //	[0] push target
                std::string,    //	[1] account
                std::string,    //	[2] number
                std::string,    //	[3] version
                std::string,    //	[4] device id
                std::uint16_t); //	[5] timeout

            [[nodiscard]] cyng::buffer_t res_open_push_channel(
                sequence_t,     //	[0] ipt seq
                response_t,     //	[1] response value
                std::uint32_t,  //	[2] channel
                std::uint32_t,  //	[3] source
                std::uint16_t,  //	[4] packet size
                std::uint8_t,   //	[5] window size
                std::uint8_t,   //	[6] status
                std::uint32_t); //	[7] count

            [[nodiscard]] std::pair<cyng::buffer_t, sequence_t> req_close_push_channel(std::uint32_t channel);

            [[nodiscard]] cyng::buffer_t
            res_close_push_channel(sequence_t const seq, response_t const res, std::uint32_t const channel);

            [[nodiscard]] cyng::buffer_t req_transfer_push_data(
                std::uint32_t, //	[0] channel
                std::uint32_t, //	[1] source
                std::uint8_t,  //	[2] status
                std::uint8_t,  //	[3] block
                cyng::buffer_t data);

            [[nodiscard]] cyng::buffer_t res_transfer_push_data(
                sequence_t,    //	[0] ipt seq
                response_t,    //	[1] response value
                std::uint32_t, //	[2] channel
                std::uint32_t, //	[3] source
                std::uint8_t,  //	[4] status
                std::uint8_t); //	[5] block

            [[nodiscard]] std::pair<cyng::buffer_t, sequence_t> req_open_connection(std::string number);

            [[nodiscard]] cyng::buffer_t res_open_connection(sequence_t seq, response_t res);

            [[nodiscard]] cyng::buffer_t req_close_connection();

            [[nodiscard]] cyng::buffer_t res_close_connection(sequence_t seq, response_t res);

            [[nodiscard]] cyng::buffer_t req_protocol_version();

            [[nodiscard]] cyng::buffer_t res_protocol_version(sequence_t seq, response_t res);

            [[nodiscard]] cyng::buffer_t req_software_version();

            [[nodiscard]] cyng::buffer_t res_software_version(sequence_t seq, std::string ver);

            [[nodiscard]] cyng::buffer_t req_device_id();

            [[nodiscard]] cyng::buffer_t res_device_id(sequence_t seq, std::string id);

            [[nodiscard]] cyng::buffer_t req_network_status();

            [[nodiscard]] cyng::buffer_t res_network_status(
                sequence_t seq,
                std::uint8_t dev,
                std::uint32_t stat_1,
                std::uint32_t stat_2,
                std::uint32_t stat_3,
                std::uint32_t stat_4,
                std::uint32_t stat_5,
                std::string imsi,
                std::string imei);

            [[nodiscard]] cyng::buffer_t req_ip_statistics();

            [[nodiscard]] cyng::buffer_t res_ip_statistics(sequence_t seq, response_t res, std::uint64_t rx, std::uint64_t sx);

            [[nodiscard]] cyng::buffer_t req_device_auth();

            [[nodiscard]] cyng::buffer_t
            res_device_auth(sequence_t seq, std::string account, std::string password, std::string number, std::string description);

            [[nodiscard]] cyng::buffer_t req_device_time();

            [[nodiscard]] cyng::buffer_t res_device_time(sequence_t seq);

            [[nodiscard]] std::pair<cyng::buffer_t, sequence_t>
            req_register_push_target(std::string target, std::uint16_t p_size, std::uint8_t w_size);

            [[nodiscard]] cyng::buffer_t res_register_push_target(sequence_t seq, response_t res, std::uint32_t channel);

            [[nodiscard]] cyng::buffer_t req_deregister_push_target(std::string target);

            [[nodiscard]] cyng::buffer_t res_deregister_push_target(sequence_t seq, response_t res, std::string target);

            [[nodiscard]] cyng::buffer_t res_unknown_command(sequence_t seq, command_t cmd);

            /**
             * set new scramble key
             */
            void set_sk(scramble_key const &key);

            /**
             * @return last sequence id
             */
            sequence_t push_seq();

            /**
             * take a buffer and scramble it's content
             * @see append()
             */
            [[nodiscard]] cyng::buffer_t scramble(cyng::buffer_t &&data);

            /**
             * Scramble and escape data.
             * In IP-T layer data bytes should not contain single
             * escape values (0x1b).
             */
            [[nodiscard]] cyng::buffer_t escape_data(cyng::buffer_t &&data);

            /**
             * @return current scramble key
             */
            scramble_key::key_type const &get_sk() const;
            std::size_t get_scrambler_index() const;

          private:
            [[nodiscard]] std::deque<cyng::buffer_t> write_header(code cmd, sequence_t seq, std::size_t length);

            /**
             * Append a '\0' value to terminate string
             */
            [[nodiscard]] cyng::buffer_t write(std::string const &);
            [[nodiscard]] cyng::buffer_t write(scramble_key::key_type const &);

            /**
             * append numeric value using the scramble key
             */
            template <typename T> [[nodiscard]] cyng::buffer_t write_numeric(T v) {
                static_assert(std::is_arithmetic<T>::value, "arithmetic type required");
                return scramble(cyng::to_buffer_be(v));
            }

            void reset();

          private:
            /**
             * produces consecutive sequence numbers
             * between 1 and 0xff (0 is excluded)
             */
            seq_generator sgen_;
            sequence_t last_seq_;

            /**
             * Encrypting data stream
             */
            scrambler_t scrambler_;
            scramble_key def_key_; //!< default scramble key
        };

        /**
         * merge multiple entries into one buffer
         */
        [[nodiscard]] cyng::buffer_t merge(std::deque<cyng::buffer_t> deq);

    } // namespace ipt
} // namespace smf

#endif
