/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_READER_H
#define SMF_MBUS_READER_H

#include <smf/mbus/dif.h>
#include <smf/mbus/vif.h>

#include <cyng/obj/intrinsics/buffer.h>
#include <cyng/obj/intrinsics/obis.h>
#include <cyng/obj/object.h>

#include <chrono>
#include <cstdint>
#include <tuple>

namespace smf {
    namespace mbus {
        using reader_rt = std::tuple<std::size_t, cyng::obis, cyng::object, std::int8_t, unit, bool>;

        /**
         * read a buffer from the specified offset and return the next vif
         */
        reader_rt read(cyng::buffer_t const &, std::size_t offset, std::uint8_t medium);

        /**
         * get the size of the value in bytes
         */
        std::size_t calculate_size(data_field_code, char);

        /**
         * convert 4 bytes to a date time
         */
        std::chrono::system_clock::time_point convert_to_tp(char, char, char, char);

        /**
         * convert 2 bytes to a date
         */
        std::chrono::system_clock::time_point convert_to_tp(char, char);

        /**
         * produce an integer value
         */
        cyng::object make_i24_value(char, char, char);

        cyng::obis make_obis(std::uint8_t medium, std::uint8_t tariff, vif const &);

    } // namespace mbus
} // namespace smf

#endif
