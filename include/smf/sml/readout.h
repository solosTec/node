/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_READOUT_H
#define SMF_SML_READOUT_H

#include <cyng/obj/intrinsics/container.h>
#include <cyng/obj/intrinsics/obis.h>
#include <cyng/obj/numeric_cast.hpp>
#include <cyng/obj/object.h>

#include <boost/asio/ip/address.hpp>

#include <cstdint>

namespace smf {
    namespace sml {

        /**
         * Read an object as buffer and initialize
         * an OBIS code with the buffer.
         * If buffer has a length other then 6 and empty OBIS
         * code will be produced.
         */
        cyng::obis read_obis(cyng::object obj);

        /**
         * SML defines 2 representations of time: TIME_TIMESTAMP and TIME_SECINDEX.
         * This functions decodes the type and produces an object that contains the
         * value. If the type is TIME_TIMESTAMP the return value is a std::chrono::system_clock::time_point.
         * Otherwise the value is an u32 value.
         *
         * Precondition is that the object contains a list/tuple with the a size of 2.
         */
        cyng::object read_time(cyng::object obj);

        /**
         * Convert the binary buffer into a string
         */
        std::string to_string(cyng::object obj);
        cyng::object read_string(cyng::object obj);

        /**
         * Convert data types of some specific OBIS values
         */
        cyng::object customize_value(cyng::obis code, cyng::object obj);

        /**
         * Expects an tuple of buffers that will be converted
         * into an OBIS path
         */
        cyng::obis_path_t read_param_tree_path(cyng::object);

        /**
         * Takes the type information (code, scaler, unit) and transform the obj parameter
         * in a meaningfull string. All calculations for positionioning the decimal point
         * will done.
         */
        cyng::object read_value(cyng::obis code, std::int8_t scaler, std::uint8_t unit, cyng::object obj);

        // cyng::param_t read_param_tree(std::size_t, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

        /**
         * A SML parameter consists of a type (PROC_PAR_VALUE, PROC_PAR_TIME, ...) and a value.
         * This functions read the parameter and creates an attribute that contains these
         * two informations
         *
         * Precondition is that the object contains a list/tuple with the a size of 2.
         */
        cyng::attr_t read_parameter(cyng::object);

        /**
         * convinience function
         */
        template <typename T> inline T read_numeric(cyng::object obj) { return cyng::numeric_cast<T>(obj, 0); }

    } // namespace sml

    std::string ip_address_to_str(cyng::object obj);
    boost::asio::ip::address to_ip_address_v4(cyng::object obj);

    std::string scale_value(std::int64_t value, std::int8_t scaler);

} // namespace smf
#endif
