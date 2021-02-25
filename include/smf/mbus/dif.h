/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_DIF_H
#define SMF_MBUS_DIF_H

#include <cstdint>

namespace smf
{
	namespace mbus	
	{
		enum data_field_code : std::uint8_t {
			DFC_NO_DATA = 0,
			DFC_8_BIT_INT = 1,	//	8 Bit Integer
			DFC_16_BIT_INT = 2,	//	16 Bit Integer
			DFC_24_BIT_INT = 3,	//	24 Bit Integer
			DFC_32_BIT_INT = 4,	//	32 Bit Integer
			DFC_32_BIT_REAL = 5,	//	32 Bit Real
			DFC_48_BIT_INT = 6,	//	48 Bit Integer
			DFC_64_BIT_INT = 7,	//	64 Bit Integer
			DFC_READOUT = 8,	//	Selection for Readout
			DFC_2_DIGIT_BCD = 9,	//	2 digit BCD
			DFC_4_DIGIT_BCD = 10,	// 	4 digit BCD
			DFC_6_DIGIT_BCD = 11,	//	5 digit BCD
			DFC_8_DIGIT_BCD = 12,	//	8 digit BCD
			DFC_VAR = 13,	// variable length - The length of the data is given with the first byte of data, which is here called LVAR.
			DFC_12_DIGIT_BCD = 14,	// 12 digit BCD
			DFC_SPECIAL = 15,	// special functions
		};

		/**
		 * Take a DIF value and returns the enum.
		 */
		constexpr std::uint8_t decode_dif(std::uint8_t val) {
			return static_cast<std::uint8_t>(val);
		}

		enum function_field_code : std::uint8_t {
			FFC_INSTANT = 0,	//	Instantaneous value
			FFC_MAX_VALUE = 1,
			FFC_MIN_VALUE = 2,
			FFC_ERR_VALUE = 3,	//	Value during error state
		};

	}
}


#endif
