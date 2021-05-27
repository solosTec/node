#include <smf/mbus/bcd.hpp>
#include <smf/mbus/reader.h>

#include <cyng/io/ostream.h>
#include <cyng/obj/buffer_cast.hpp>
#include <cyng/obj/factory.hpp>

#include <ctime>
#include <iostream>
#include <iterator> // for back_inserter

#include <boost/assert.hpp>
#include <boost/predef.h>

namespace smf {
    namespace mbus {
        /**
         * read a buffer from the specified offset and return the next vif
         */
        reader_rt read(cyng::buffer_t const &data, std::size_t offset, std::uint8_t medium) {

            BOOST_ASSERT(data.size() > offset);
            if (data.size() > offset) {

                //	check dummy bytes
                if (data.at(offset) == static_cast<char>(0x2f)) {
                    return std::make_tuple(data.size(), cyng::obis(), cyng::make_object(), 0, unit::UNDEFINED_);
                }

                //
                //	read dif
                //
                dif const d(data.at(offset));
                ++offset;
                BOOST_ASSERT(data.size() > offset);

                std::uint8_t tariff = 0;

                // const char* get_name(data_field_code dfc)
                std::cout << get_name(d.get_data_field_code()) << std::endl;

                if (d.is_extended()) {
                    //
                    //	read dife
                    //
                    dife const de(data.at(offset));
                    ++offset;
                    BOOST_ASSERT(data.size() > offset);

                    //
                    //	update tariff
                    //
                    tariff = de.get_tariff();
                }

                //
                //	read vif
                //
                vif const v(data.at(offset));
                ++offset;
                BOOST_ASSERT(data.size() > offset);

                if (v.is_extended()) {
                    //
                    //	read vife
                    //
                    ++offset;
                    BOOST_ASSERT(data.size() > offset);
                }

                auto const size = calculate_size(d.get_data_field_code(), data.at(offset));
                BOOST_ASSERT(data.size() >= offset + size);

                if (data_field_code::DFC_VAR == d.get_data_field_code()) {
                    ++offset;
                    BOOST_ASSERT(data.size() > offset + size);
                }

                //
                //	get vib type
                //  ToDo: data type DFC_VAR could overwrite the data type
                //
                auto const vt = v.get_vib_type();

                //
                //	read value
                //
                if (data.size() > offset + size) {

                    cyng::buffer_t value;
                    value.reserve(size);
                    std::copy_n(data.begin() + offset, size, std::back_inserter(value));
                    offset += size;

                    switch (d.get_data_field_code()) {
                    case data_field_code::DFC_NO_DATA:
                        break;
                    case data_field_code::DFC_8_BIT_INT:
                        std::cout << cyng::to_numeric_be<std::int8_t>(value) << std::endl;
                        break;
                    case data_field_code::DFC_16_BIT_INT:
                        BOOST_ASSERT(value.size() == 2);
                        if (value.size() == 2) {
                            if (v.is_date()) {
                                return std::make_tuple(
                                    offset,
                                    cyng::make_obis(
                                        1, 0, 0, 9, 2 //	date
                                        ,
                                        255),
                                    cyng::make_object(convert_to_tp(value.at(0), value.at(1))), vt.first, vt.second);
                            } else {

                                std::cout << cyng::to_numeric_be<std::int16_t>(value) << std::endl;
                                return std::make_tuple(
                                    offset, make_obis(medium, tariff, v),
                                    cyng::make_object(cyng::to_numeric_be<std::int16_t>(value)), vt.first, vt.second);
                            }
                        }
                        break;
                    case data_field_code::DFC_24_BIT_INT:
                        BOOST_ASSERT(value.size() == 3);
                        if (value.size() == 3) {
                            //std::cout << cyng::to_numeric_be<std::int32_t>(value) << std::endl;
                            return std::make_tuple(
                                offset, cyng::obis(), cyng::make_object(cyng::to_numeric_be<std::int32_t>(value)),
                                vt.first, vt.second);
                        }
                        break;
                    case data_field_code::DFC_32_BIT_INT:
                        BOOST_ASSERT(value.size() == 4);
                        if (value.size() == 4) {
                            if (v.is_time()) {
                                return std::make_tuple(
                                    offset,
                                    cyng::make_obis(
                                        1, 0, 0, 9, 1, //	time
                                        255),
                                    cyng::make_object(convert_to_tp(value.at(0), value.at(1), value.at(2), value.at(3))), vt.first,
                                    vt.second);
                            } else {
                                std::cout << cyng::to_numeric_be<std::int32_t>(value) << std::endl;
                                return std::make_tuple(
                                    offset, cyng::obis(), cyng::make_object(cyng::to_numeric_be<std::int32_t>(value)), vt.first,
                                    vt.second);
                            }
                        }
                        break;
                    case data_field_code::DFC_32_BIT_REAL:
                        BOOST_ASSERT(value.size() == 4);
                        if (value.size() == 4) {
                            //	IEEE 754
                            //	example: 0x42 0xD7 0xE3 0xB4 => = 107.945 (107.944733)
                            //  note: byte ordering has to be reversed
                            auto const f = cyng::to_numeric_be<float>(value);
                            std::cout << f << std::endl;
                            return std::make_tuple(offset, cyng::obis(), cyng::make_object(f), vt.first, vt.second);
                        }
                        break;
                    case data_field_code::DFC_48_BIT_INT:
                        break;
                    case data_field_code::DFC_64_BIT_INT:
                        BOOST_ASSERT(value.size() == 8);
                        if (value.size() == 8) {
                            std::cout << cyng::to_numeric_be<std::int64_t>(value) << std::endl;
                            return std::make_tuple(
                                offset, cyng::obis(), cyng::make_object(cyng::to_numeric_be<std::int64_t>(value)), vt.first,
                                vt.second);
                        }
                        break;
                    case data_field_code::DFC_READOUT:
                        break;
                    case data_field_code::DFC_2_DIGIT_BCD:
                        BOOST_ASSERT(value.size() == 1);
                        if (value.size() == 1) {
                            std::cout << bcd_to_n<std::uint16_t>(value) << std::endl;
                            return std::make_tuple(
                                offset, cyng::obis(), cyng::make_object(bcd_to_n<std::uint16_t>(value)), vt.first, vt.second);
                        }
                        break;
                    case data_field_code::DFC_4_DIGIT_BCD:
                        BOOST_ASSERT(value.size() == 2);
                        if (value.size() == 2) {
                            std::cout << bcd_to_n<std::uint32_t>(value) << std::endl;
                            return std::make_tuple(
                                offset, cyng::obis(), cyng::make_object(bcd_to_n<std::uint32_t>(value)), vt.first, vt.second);
                        }
                        break;
                    case data_field_code::DFC_6_DIGIT_BCD:
                        BOOST_ASSERT(value.size() == 3);
                        if (value.size() == 3) {
                            std::cout << bcd_to_n<std::uint32_t>(value) << std::endl;
                            return std::make_tuple(
                                offset, cyng::obis(), cyng::make_object(bcd_to_n<std::uint32_t>(value)), vt.first, vt.second);
                        }
                        break;
                    case data_field_code::DFC_8_DIGIT_BCD:
                        BOOST_ASSERT(value.size() == 4);
                        if (value.size() == 4) {
                            std::cout << bcd_to_n<std::uint64_t>(value) << std::endl;
                            return std::make_tuple(
                                offset, cyng::obis(), cyng::make_object(bcd_to_n<std::uint64_t>(value)), vt.first, vt.second);
                        }
                        break;
                    case data_field_code::DFC_VAR:
                        break;
                    case data_field_code::DFC_12_DIGIT_BCD:
                        BOOST_ASSERT(value.size() == 6);
                        if (value.size() == 6) {
                            std::cout << bcd_to_n<std::uint64_t>(value) << std::endl;
                            return std::make_tuple(
                                offset, cyng::obis(), cyng::make_object(bcd_to_n<std::uint64_t>(value)), vt.first, vt.second);
                        }
                        break;
                    case data_field_code::DFC_SPECIAL:
                    default:
                        break;
                    }
                    // return "unknown";
                    // if (is_bcd(d.get_data_field_code())) {
                    //	std::cout << smf::mbus::bcd_to_n<std::uint32_t>(value)
                    //<< std::endl;
                    //}
                }

                return std::make_tuple(offset, cyng::obis(), cyng::make_object(), vt.first, vt.second);
            }
            return std::make_tuple(data.size(), cyng::obis(), cyng::make_object(), 0, unit::UNDEFINED_);
        }

        cyng::obis make_obis(std::uint8_t medium, std::uint8_t tariff, vif const &v) {
            // VG_MEDIUM = 0,		//	A
            // VG_CHANNEL = 1,		//	B
            // VG_INDICATOR = 2,	//	C - metric (physcial value)
            // VG_MODE = 3,		//	D - measurement mode
            // VG_QUANTITY = 4,	//	E - tariff
            // VG_STORAGE = 5,		//	F

            return cyng::make_obis(medium, 0, 1, 8, tariff, 255);
        }

        std::size_t calculate_size(data_field_code dfc, char c) {

            if (data_field_code::DFC_VAR == dfc) {
                if (c < 0xC0) {
                    //  0x00 .. 0xBF: ASCII
                    //	max 192 characters
                    return static_cast<std::size_t>(c);
                } else if (c < 0xD0) {
                    //	BCD_POSITIVE
                    return static_cast<std::size_t>((c - 0xC0) * 2);
                } else if (c < 0xE0) {
                    //	BCD_NEGATIVE_;
                    return static_cast<std::size_t>((c - 0xD0) * 2);
                } else if (c < 0xF0) {
                    //	DATA_BINARY
                    return static_cast<std::size_t>(c - 0xE0);
                } else if (c < 0xFB) {
                    //	DATA_FP - floating point number
                    return static_cast<std::size_t>(c - 0xF0);
                }
                //	reserved
                return static_cast<std::size_t>(c - 0xFB);

            } else if (data_field_code::DFC_READOUT == dfc) {
                BOOST_ASSERT_MSG(false, "not implemented");

            } else if (data_field_code::DFC_SPECIAL == dfc) {
                BOOST_ASSERT_MSG(false, "not implemented");
            }
            return get_length(dfc);
        }

        std::chrono::system_clock::time_point convert_to_tp(char a, char b, char c, char d) {

            std::tm t{0};
            t.tm_isdst = -1; // daylight savings time flag [-1/0/1]

            t.tm_min = (a & 0x3f);
            t.tm_hour = (b & 0x1f);
            t.tm_mday = (c & 0x1f);
            t.tm_mon = (d & 0x0f) - 1; //	 [0, 11]

            int yearh = (0x60 & b) >> 5;
            int year1 = (0xe0 & c) >> 5;
            int year2 = (0xf0 & d) >> 1;

            if (yearh == 0) {
                yearh = 1;
            }

            //	years since 1900 (108 => 2008)
            t.tm_year = (100 * yearh) + year1 + year2; //	years since 1900

#if defined(BOOST_OS_WINDOWS_AVAILABLE)
            return std::chrono::system_clock::from_time_t(::_mkgmtime(&t));
#else
            //	nonstandard GNU extension, also present on the BSDs
            return std::chrono::system_clock::from_time_t(::timegm(&t));
#endif
        }

        std::chrono::system_clock::time_point convert_to_tp(char a, char b) {

            std::tm t{0};

            t.tm_mday = (0x1f & a);
            t.tm_mon = (b & 0x0f) - 1; //	 [0, 11]

            int year1 = ((0xe0) & a) >> 5;
            int year2 = ((0xf0) & b) >> 1;
            t.tm_year = (100 + year1 + year2);

#if defined(BOOST_OS_WINDOWS_AVAILABLE)
            return std::chrono::system_clock::from_time_t(::_mkgmtime(&t));
#else
            //	nonstandard GNU extension, also present on the BSDs
            return std::chrono::system_clock::from_time_t(::timegm(&t));
#endif
        }

        cyng::object make_i24_value(char a, char b, char c) {

            if ((c & 0x80) == 0x80) {
                // negative
                std::int32_t val = a | (b << 8) | (c << 16) | 0xff << 24;
                return cyng::make_object(val);
            } else {
                // positive
                std::uint32_t val = a | (b << 8) | (c << 16);
                return cyng::make_object(val);
            }
        }

    } // namespace mbus
} // namespace smf
