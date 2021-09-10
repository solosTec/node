/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_VALUE_HPP
#define SMF_SML_VALUE_HPP

#include <cyng/obj/container_factory.hpp>
#include <cyng/obj/intrinsics/aes_key.hpp>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/obis.h>

#include <cstdint>

namespace smf {

    namespace sml {

        enum proc_par_value : std::uint8_t {
            PROC_PAR_UNDEF = 0,
            PROC_PAR_VALUE = 1,
            PROC_PAR_PERIODENTRY = 2,
            PROC_PAR_TUPELENTRY = 3,
            PROC_PAR_TIME = 4,
        };

        /**
         *	time value types
         */
        enum sml_time_enum : std::uint8_t {
            TIME_SECINDEX = 1,
            TIME_TIMESTAMP = 2,
        };

        namespace detail {
            template <typename T> struct factory_policy {};

            template <> struct factory_policy<bool> { static cyng::tuple_t create(bool b); };
            template <> struct factory_policy<std::uint8_t> { static cyng::tuple_t create(std::uint8_t v); };
            template <> struct factory_policy<std::uint16_t> { static cyng::tuple_t create(std::uint16_t v); };
            template <> struct factory_policy<std::uint32_t> { static cyng::tuple_t create(std::uint32_t v); };
            template <> struct factory_policy<std::uint64_t> { static cyng::tuple_t create(std::uint64_t v); };
            template <> struct factory_policy<std::int8_t> { static cyng::tuple_t create(std::int8_t v); };
            template <> struct factory_policy<std::int16_t> { static cyng::tuple_t create(std::int16_t v); };
            template <> struct factory_policy<std::int32_t> { static cyng::tuple_t create(std::int32_t v); };
            template <> struct factory_policy<std::int64_t> { static cyng::tuple_t create(std::int64_t v); };

            template <> struct factory_policy<std::chrono::system_clock::time_point> {
                static cyng::tuple_t create(std::chrono::system_clock::time_point &&v);
                static cyng::tuple_t create(std::chrono::system_clock::time_point const &v);
            };
            template <> struct factory_policy<std::chrono::milliseconds> {
                static cyng::tuple_t create(std::chrono::milliseconds v);
            };
            template <> struct factory_policy<std::chrono::seconds> { static cyng::tuple_t create(std::chrono::seconds v); };
            template <> struct factory_policy<std::chrono::minutes> { static cyng::tuple_t create(std::chrono::minutes v); };

            template <> struct factory_policy<cyng::buffer_t> {
                static cyng::tuple_t create(cyng::buffer_t &&v);
                static cyng::tuple_t create(cyng::buffer_t const &v);
            };
            template <> struct factory_policy<std::string> { static cyng::tuple_t create(std::string v); };

            template <> struct factory_policy<char const *> {
                static cyng::tuple_t create(char const *p) { return factory_policy<std::string>::create(std::string(p)); }
            };

            template <> struct factory_policy<cyng::obis> { static cyng::tuple_t create(cyng::obis v); };

            template <> struct factory_policy<boost::asio::ip::address> {
                static cyng::tuple_t create(boost::asio::ip::address v);
            };
            template <> struct factory_policy<boost::asio::ip::address_v4> {
                static cyng::tuple_t create(boost::asio::ip::address_v4 v);
            };
            template <> struct factory_policy<boost::asio::ip::address_v6> {
                static cyng::tuple_t create(boost::asio::ip::address_v6 v);
            };

            template <std::size_t N> struct factory_policy<cyng::aes_key<N>> {
                static cyng::tuple_t create(cyng::aes_key<N> const &key) {
                    return factory_policy<cyng::buffer_t>::create(cyng::to_buffer(key));
                }
            };

        } // namespace detail

        template <typename T> cyng::tuple_t make_value(T &&v) {
            //
            //	When using decay<T> to get the plain data type we cannot longer
            //	detect string literals. Without decay<T> we have to handle each
            //	variation with const and & individually.
            //

            // using type = typename std::remove_const<T>::type;
            using type = std::remove_cv_t<std::remove_reference_t<T>>;
            return detail::factory_policy<type>::create(std::forward<T>(v));
        }

        template <std::size_t N> cyng::tuple_t make_value(const char (&p)[N]) {
            return detail::factory_policy<std::string>::create(std::string(p, N - 1));
        };

        /**
         * SML Time of type 3 is used but not defined in SML v1.03
         */
        cyng::tuple_t make_local_timestamp(std::chrono::system_clock::time_point);

        /**
         * Make a timestamp as type TIME_TIMESTAMP (2)
         */
        cyng::tuple_t make_timestamp(std::chrono::system_clock::time_point);

        /**
         * Make a timestamp as type TIME_SECINDEX (1)
         */
        cyng::tuple_t make_sec_index(std::chrono::system_clock::time_point);

        /**
         * generate an empty value
         */
        cyng::tuple_t make_value();

        /**
         * Convert an object to a SML value.
         * Works for simple data types only.
         */
        cyng::tuple_t to_value(cyng::object);

    } // namespace sml
} // namespace smf
#endif
