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


