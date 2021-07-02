/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/mbus/dif.h>

#include <boost/assert.hpp>

namespace smf {
    namespace mbus {
        const char *get_name(data_field_code dfc) {
            switch (dfc) {
            case data_field_code::DFC_NO_DATA:
                return "DFC_NO_DATA";
            case data_field_code::DFC_8_BIT_INT:
                return "DFC_8_BIT_INT";
            case data_field_code::DFC_16_BIT_INT:
                return "DFC_16_BIT_INT";
            case data_field_code::DFC_24_BIT_INT:
                return "DFC_24_BIT_INT";
            case data_field_code::DFC_32_BIT_INT:
                return "DFC_32_BIT_INT";
            case data_field_code::DFC_32_BIT_REAL:
                return "DFC_32_BIT_REAL";
            case data_field_code::DFC_48_BIT_INT:
                return "DFC_48_BIT_INT";
            case data_field_code::DFC_64_BIT_INT:
                return "DFC_64_BIT_INT";
            case data_field_code::DFC_READOUT:
                return "DFC_READOUT";
            case data_field_code::DFC_2_DIGIT_BCD:
                return "DFC_2_DIGIT_BCD";
            case data_field_code::DFC_4_DIGIT_BCD:
                return "DFC_4_DIGIT_BCD";
            case data_field_code::DFC_6_DIGIT_BCD:
                return "DFC_6_DIGIT_BCD";
            case data_field_code::DFC_8_DIGIT_BCD:
                return "DFC_8_DIGIT_BCD";
            case data_field_code::DFC_VAR:
                return "DFC_VAR";
            case data_field_code::DFC_12_DIGIT_BCD:
                return "DFC_12_DIGIT_BCD";
            case data_field_code::DFC_SPECIAL:
                return "DFC_SPECIAL";
            default:
                break;
            }
            return "unknown";
        }

        dif::dif(char c)
            : value_(1, static_cast<std::uint8_t>(c)) {}

        dif::dif(std::uint8_t c)
            : value_(1, c) {}

        std::uint8_t dif::get_value() const {
            BOOST_ASSERT(!value_.empty());
            return value_.at(0);
        }

        std::uint8_t dif::get_and(std::uint8_t v) const { return get_value() & v; }

        std::uint8_t dif::get_last() const {
            BOOST_ASSERT(!value_.empty());
            return value_.back();
        }

        data_field_code dif::get_data_field_code() const { return decode_dfc(get_value()); }

        function_field_code dif::get_function_field_code() const { return decode_ffc(get_value()); }

        bool dif::is_storage() const { return get_and(0x40) == 0x40; }

        std::uint8_t dif::get_storage_nr() const { return (is_storage()) ? (get_last() & 0x04) : 0; }

        bool dif::is_extended() const { return get_and(0x80) == 0x80; }

        std::size_t dif::get_dife_size() const { return value_.size() - 1; }

        bool dif::is_complete() const { return !((get_last() & 0x80) == 0x80); }

        std::uint8_t dif::get_tariff() const { return (is_extended()) ? (get_last() & 0x30) >> 4 : 0; }

        std::size_t dif::add(char c) {
            value_.push_back(c);
            return value_.size();
        }

    } // namespace mbus
} // namespace smf
