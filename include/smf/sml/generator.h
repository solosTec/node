/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_GENERATOR_H
#define SMF_SML_GENERATOR_H

#include <smf/sml/attention.h>
#include <smf/sml/msg.h>

#include <cyng/obj/intrinsics/mac.h>
#include <cyng/obj/intrinsics/obis.h>
#include <cyng/rnd/rnd.hpp>

#include <chrono>

namespace smf {
    namespace sml {

        /**
         * Generate SML transaction is with the following format:
         * @code
         * [7 random numbers] [1 number in ascending order].
         * nnnnnnnnnnnnnnnnn-m
         * @endcode
         *
         * The numbers as the ASCII codes from 48 up to 57.
         *
         * In a former version the following format was used:
         * @code
         * [17 random numbers] ['-'] [1 number in ascending order].
         * nnnnnnnnnnnnnnnnn-m
         * @endcode
         *
         * This is to generate a series of continuous transaction IDs
         * and after reshuffling to start a new series.
         */
        class trx {
          public:
            trx();
            trx(trx const &);

            //~trx();

            /**
             *	generate new transaction id (reshuffling)
             */
            void reset(std::size_t);
            void reset();

            /**
             * Increase internal transaction number.
             */
            trx &operator++();
            trx operator++(int);

            /**
             *	@return value of last generated (shuffled) transaction id
             *	as string.
             */
            std::string operator*() const;

            /**
             *	@return current prefix
             */
            std::string const &get_prefix() const;

            /**
             * store current prefix
             */
            std::size_t store_pefix();
            std::pair<std::size_t, bool> ack_trx(std::string const &);

          private:
            /**
             * Generate a random string
             */
            cyng::crypto::rnd gen_;

            /**
             * The fixed part of a series
             */
            std::string prefix_;

            /**
             * Ascending number
             */
            std::uint16_t num_;

            /**
             * set of all pending transaction prefixes
             */
            std::set<std::string> open_trx_;
        };

        /**
         * @return a file id of length 12
         */
        std::string generate_file_id();

        /**
         * Generate SML responses
         */
        class response_generator {
          public:
            response_generator();

            [[nodiscard]] cyng::tuple_t
            public_open(std::string trx, cyng::buffer_t file_id, cyng::buffer_t client, std::string server);

            [[nodiscard]] cyng::tuple_t public_close(std::string trx);

            /**
             * SML_GetProcParameter.Res
             */
            [[nodiscard]] cyng::tuple_t
            get_proc_parameter(std::string const &trx, cyng::buffer_t const &server, cyng::obis_path_t const &path, cyng::tuple_t);

            /**
             * SML_GetList.Res
             */
            [[nodiscard]] cyng::tuple_t get_list(cyng::buffer_t const &server, cyng::tuple_t val_list);

            /**
             * SML_GetProfileList.Res
             */
            [[nodiscard]] cyng::tuple_t get_profile_list(
                std::string const &trx,
                cyng::buffer_t const &server,
                std::chrono::system_clock::time_point act_time,
                std::uint32_t reg_period,
                cyng::obis,
                std::uint32_t val_time,
                std::uint64_t status,
                cyng::tuple_t);

            /**
             * Attention
             */
            [[nodiscard]] cyng::tuple_t
            get_attention(std::string const &trx, cyng::buffer_t const &server, attention_type, std::string txt);
        };

        /**
         * Generate SML requests
         */
        class request_generator {
          public:
            request_generator(std::string const &name, std::string const &pwd);
            request_generator(request_generator const &);

            void reset(std::string const &name, std::string const &pwd, std::size_t length);

            /**
             * public open request
             */
            [[nodiscard]] cyng::tuple_t public_open(cyng::mac48 client_id, cyng::buffer_t const &server_id);

            /**
             * public close request
             */
            [[nodiscard]] cyng::tuple_t public_close();

            /**
             * Restart system - 81 81 C7 83 82 01
             */
            [[nodiscard]] cyng::tuple_t set_proc_parameter_reboot(cyng::buffer_t const &server_id);

            /**
             * Simple root query - BODY_GET_PROC_PARAMETER_REQUEST (0x500)
             * @return transaction ID
             */
            [[nodiscard]] cyng::tuple_t get_proc_parameter(cyng::buffer_t const &server_id, cyng::obis);

            [[nodiscard]] cyng::tuple_t get_proc_parameter(cyng::buffer_t const &server_id, cyng::obis_path_t);

            [[nodiscard]] cyng::tuple_t get_proc_parameter_access(
                cyng::buffer_t const &server_id,
                std::uint8_t role,
                std::uint8_t user,
                std::uint16_t device_index);

            [[nodiscard]] cyng::tuple_t get_profile_list(
                cyng::buffer_t const &server_id,
                cyng::obis,
                std::chrono::system_clock::time_point start,
                std::chrono::system_clock::time_point end);

            [[nodiscard]] cyng::tuple_t set_proc_parameter(cyng::buffer_t const &server_id, cyng::obis_path_t, cyng::attr_t attr);

            std::string const &get_name() const;
            std::string const &get_pwd() const;

            /**
             *	@return transaction manager
             */
            trx &get_trx_mgr();

          private:
            trx trx_;
            /**
             * buffer for current SML message
             */
            std::uint8_t group_no_;
            std::string name_;
            std::string pwd_;
        };

    } // namespace sml
} // namespace smf
#endif
