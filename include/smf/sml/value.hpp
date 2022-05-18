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
            PROC_PAR_LIST_ENTRY = 5
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

            template <> struct factory_policy<bool> { static cyng::attr_t create(bool b); };

            template <> struct factory_policy<std::uint8_t> {
                template <typename U> static cyng::attr_t create(U v) {
                    return cyng::make_attr(PROC_PAR_VALUE, static_cast<std::uint8_t>(v));
                }
            };
            template <> struct factory_policy<std::uint16_t> {
                template <typename U> static cyng::attr_t create(U v) {
                    return cyng::make_attr(PROC_PAR_VALUE, static_cast<std::uint16_t>(v));
                }
            };
            template <> struct factory_policy<std::uint32_t> {
                template <typename U> static cyng::attr_t create(U v) {
                    return cyng::make_attr(PROC_PAR_VALUE, static_cast<std::uint32_t>(v));
                }
            };
            template <> struct factory_policy<std::uint64_t> {
                template <typename U> static cyng::attr_t create(U v) {
                    return cyng::make_attr(PROC_PAR_VALUE, static_cast<std::uint64_t>(v));
                }
            };
            template <> struct factory_policy<std::int8_t> {
                template <typename U> static cyng::attr_t create(U v) {
                    return cyng::make_attr(PROC_PAR_VALUE, static_cast<std::int8_t>(v));
                }
            };
            template <> struct factory_policy<std::int16_t> {
                template <typename U> static cyng::attr_t create(U v) {
                    return cyng::make_attr(PROC_PAR_VALUE, static_cast<std::int16_t>(v));
                }
            };
            template <> struct factory_policy<std::int32_t> {
                template <typename U> static cyng::attr_t create(U v) {
                    return cyng::make_attr(PROC_PAR_VALUE, static_cast<std::int32_t>(v));
                }
            };
            template <> struct factory_policy<std::int64_t> {
                template <typename U> static cyng::attr_t create(U v) {
                    return cyng::make_attr(PROC_PAR_VALUE, static_cast<std::int64_t>(v));
                }
            };

            template <> struct factory_policy<std::chrono::system_clock::time_point> {
                static cyng::attr_t create(std::chrono::system_clock::time_point &&v);
                static cyng::attr_t create(std::chrono::system_clock::time_point const &v);
            };
            template <> struct factory_policy<std::chrono::milliseconds> {
                static cyng::attr_t create(std::chrono::milliseconds v);
            };
            template <> struct factory_policy<std::chrono::seconds> { static cyng::attr_t create(std::chrono::seconds v); };
            template <> struct factory_policy<std::chrono::minutes> { static cyng::attr_t create(std::chrono::minutes v); };

            template <> struct factory_policy<cyng::buffer_t> {
                static cyng::attr_t create(cyng::buffer_t &&v);
                static cyng::attr_t create(cyng::buffer_t const &v);
            };
            template <> struct factory_policy<std::string> { static cyng::attr_t create(std::string v); };

            template <> struct factory_policy<char const *> {
                static cyng::attr_t create(char const *p) { return factory_policy<std::string>::create(std::string(p)); }
            };

            template <> struct factory_policy<cyng::obis> { static cyng::attr_t create(cyng::obis v); };

            template <> struct factory_policy<boost::asio::ip::address> { static cyng::attr_t create(boost::asio::ip::address v); };
            template <> struct factory_policy<boost::asio::ip::address_v4> {
                static cyng::attr_t create(boost::asio::ip::address_v4 v);
            };
            template <> struct factory_policy<boost::asio::ip::address_v6> {
                static cyng::attr_t create(boost::asio::ip::address_v6 v);
            };

            template <std::size_t N> struct factory_policy<cyng::aes_key<N>> {
                static cyng::attr_t create(cyng::aes_key<N> const &key) {
                    return factory_policy<cyng::buffer_t>::create(cyng::to_buffer(key));
                }
            };

        } // namespace detail

        /**
         * generate an empty value
         */
        [[nodiscard]] cyng::tuple_t make_value();
        [[nodiscard]] cyng::attr_t make_attribute();

        /**
         * Convert an object to a SML value.
         * Works for simple data types only.
         */
        [[nodiscard]] cyng::tuple_t to_value(cyng::object);
        [[nodiscard]] cyng::tuple_t to_value(cyng::attr_t);
        [[nodiscard]] cyng::attr_t to_attribute(cyng::object);

        template <typename T> cyng::attr_t make_attribute(T &&v) {
            //
            //	When using decay<T> to get the plain data type we cannot longer
            //	detect string literals. Without decay<T> we have to handle each
            //	variation with const and & individually.
            //
            using type = std::remove_cv_t<std::remove_reference_t<T>>;
            return detail::factory_policy<type>::create(std::forward<T>(v));
        }

        /**
         * matching C-string literals
         */
        template <std::size_t N> cyng::attr_t make_attribute(const char (&p)[N]) {
            return detail::factory_policy<std::string>::create(std::string(p, N - 1));
        };

        template <typename T> cyng::tuple_t make_value(T &&v) {
            auto attr = make_attribute(v);
            return cyng::make_tuple(static_cast<std::uint8_t>(attr.first), std::move(attr.second));
        }

        /**
         * SML Time of type 3 is used but not defined in SML v1.03
         */
        cyng::tuple_t make_local_timestamp(std::chrono::system_clock::time_point);
        cyng::tuple_t make_local_timestamp();

        /**
         * Make a timestamp as type TIME_TIMESTAMP (2)
         */
        cyng::tuple_t make_timestamp(std::chrono::system_clock::time_point);
        cyng::tuple_t make_timestamp();
        cyng::attr_t make_timestamp_attr(std::chrono::system_clock::time_point);
        cyng::attr_t make_timestamp_attr();

        /**
         * Make a timestamp as type TIME_SECINDEX (1)
         */
        cyng::tuple_t make_sec_index(std::chrono::system_clock::time_point);

    } // namespace sml
} // namespace smf
#endif
