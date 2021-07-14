/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_GENERATOR_H
#define SMF_SML_GENERATOR_H

#include <smf/sml/msg.h>

#include <cyng/obj/intrinsics/obis.h>
#include <cyng/rnd/rnd.hpp>

namespace smf {
    namespace sml {

        namespace detail {
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

                /**
                 *	generate new transaction id (reshuffling)
                 */
                void regenerate(std::size_t = 7);

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

              private:
                /**
                 * Generate a random string
                 */
                cyng::crypto::rnd gen_;

                /**
                 * The fixed part of a series
                 */
                std::string value_;

                /**
                 * Ascending number
                 */
                std::uint16_t num_;
            };

        } // namespace detail
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

          private:
            detail::trx trx_;
        };
        /**
         * Generate SML requests
         */
        class request_generator {
          public:
            request_generator(std::string const &name, std::string const &pwd);

          private:
            detail::trx trx_;
            std::string const name_;
            std::string const pwd_;
        };

    } // namespace sml
} // namespace smf
#endif
