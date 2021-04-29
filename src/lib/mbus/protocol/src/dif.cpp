/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/mbus/dif.h>

namespace smf
{
	namespace mbus
	{
		const char* get_name(data_field_code dfc) {
			switch (dfc) {
			case data_field_code::DFC_NO_DATA:		return "DFC_NO_DATA";
			case data_field_code::DFC_8_BIT_INT:	return "DFC_8_BIT_INT";
			case data_field_code::DFC_16_BIT_INT:	return "DFC_16_BIT_INT";
			case data_field_code::DFC_24_BIT_INT:	return "DFC_24_BIT_INT";
			case data_field_code::DFC_32_BIT_INT:	return "DFC_32_BIT_INT";
			case data_field_code::DFC_32_BIT_REAL:	return "DFC_32_BIT_REAL";
			case data_field_code::DFC_48_BIT_INT:	return "DFC_48_BIT_INT";
			case data_field_code::DFC_64_BIT_INT:	return "DFC_64_BIT_INT";
			case data_field_code::DFC_READOUT:		return "DFC_READOUT";
			case data_field_code::DFC_2_DIGIT_BCD:	return "DFC_2_DIGIT_BCD";
			case data_field_code::DFC_4_DIGIT_BCD:	return "DFC_4_DIGIT_BCD";
			case data_field_code::DFC_6_DIGIT_BCD:	return "DFC_6_DIGIT_BCD";
			case data_field_code::DFC_8_DIGIT_BCD:	return "DFC_8_DIGIT_BCD";
			case data_field_code::DFC_VAR:			return "DFC_VAR";
			case data_field_code::DFC_12_DIGIT_BCD:	return "DFC_12_DIGIT_BCD";
			case data_field_code::DFC_SPECIAL:		return "DFC_SPECIAL";
			default:
				break;
			}
			return "unknown";

		}

		dif::dif(char c)
			: value_(static_cast<std::uint8_t>(c))
		{}

		dif::dif(std::uint8_t c)	
			: value_(c)
		{}

		
		data_field_code dif::get_data_field_code() const {
			return decode_dfc(value_);
		}

		function_field_code dif::get_function_field_code() const {
			return decode_ffc(value_);
		}

		bool dif::is_storage() const {
			return (value_ & 0x40) == 0x40;
		}

		bool dif::is_extended() const {
			return (value_ & 0x80) == 0x80;
		}

		dife::dife(char c)
			: value_(static_cast<std::uint8_t>(c))
		{}

		dife::dife(std::uint8_t c)
			: value_(c)
		{}

		std::uint8_t dife::get_storage_nr() const {
			return value_ & 0x04;
		}

		std::uint8_t dife::get_tariff() const {
			return (value_ & 0x30) >> 4;
		}

		bool dife::is_extended() const {
			return (value_ & 0x80) == 0x80;
		}


	}
}


