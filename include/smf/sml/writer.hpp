/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2032 Sylko Olzscher
 *
 */

#include <cyng/io/io.h>
#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/eod.h>
#include <cyng/obj/intrinsics/mac.h>
#include <cyng/obj/intrinsics/obis.h>

#include <chrono>

namespace cyng {
    namespace io {

        /**
         * Serialization to SML.
         */
        struct SML {};

        template <typename T> struct serializer<T, SML> {
            static std::size_t write(std::ostream &os, T const &v) {
                auto const pos = os.tellp();
                //	optional
                os.put(0x01); //	TL field without data
                return os.tellp() - pos;
            }
        };

        template <> struct serializer<eod, SML> { static std::size_t write(std::ostream &os, cyng::eod v); };

        template <> struct serializer<bool, SML> { static std::size_t write(std::ostream &os, bool v); };

        template <> struct serializer<std::uint8_t, SML> { static std::size_t write(std::ostream &os, std::uint8_t v); };

        template <> struct serializer<std::uint16_t, SML> { static std::size_t write(std::ostream &os, std::uint16_t v); };

        template <> struct serializer<std::uint32_t, SML> { static std::size_t write(std::ostream &os, std::uint32_t v); };

        template <> struct serializer<std::uint64_t, SML> { static std::size_t write(std::ostream &os, std::uint64_t v); };

        template <> struct serializer<std::int8_t, SML> { static std::size_t write(std::ostream &os, std::int8_t v); };

        template <> struct serializer<std::int16_t, SML> { static std::size_t write(std::ostream &os, std::int16_t v); };

        template <> struct serializer<std::int32_t, SML> { static std::size_t write(std::ostream &os, std::int32_t v); };

        template <> struct serializer<std::int64_t, SML> { static std::size_t write(std::ostream &os, std::int64_t v); };

        template <> struct serializer<cyng::buffer_t, SML> { static std::size_t write(std::ostream &os, cyng::buffer_t const &v); };

        template <> struct serializer<std::string, SML> { static std::size_t write(std::ostream &os, std::string const &v); };

        template <> struct serializer<std::chrono::system_clock::time_point, SML> {
            static std::size_t write(std::ostream &os, std::chrono::system_clock::time_point v);
        };

        template <> struct serializer<cyng::date, SML> {
            static std::size_t write(std::ostream &os, cyng::date v);
        };

        template <> struct serializer<cyng::tuple_t, SML> {
            static std::size_t write(std::ostream &os, cyng::tuple_t const &v);
        };

        /**
         * serialize as a tuple with two elements, the index (u8) and the value itself.
         */
        template <> struct serializer<cyng::attr_t, SML> { static std::size_t write(std::ostream &os, cyng::attr_t const &v); };

        template <> struct serializer<cyng::obis, SML> { static std::size_t write(std::ostream &os, cyng::obis const &v); };

        template <> struct serializer<cyng::obis_path_t, SML> {
            static std::size_t write(std::ostream &os, cyng::obis_path_t const &v);
        };

        template <> struct serializer<cyng::mac48, SML> { static std::size_t write(std::ostream &os, cyng::mac48 const &v); };

#ifdef _DEBUG_UNIT_TEST
        std::uint8_t get_shift_count(std::uint32_t length);
        std::vector<std::uint8_t> write_data_length_field(std::uint32_t length, std::uint8_t type);
        void write_length_field(std::ostream &os, std::uint32_t length, std::uint8_t type);
#endif

    } // namespace io
} // namespace cyng
