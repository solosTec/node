/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SML_MBUS_DIF_H
#define NODE_SML_MBUS_DIF_H

#include <cstdint>

#pragma pack(push, 1)

namespace node
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

		enum function_field_code : std::uint8_t {
			FFC_INSTANT = 0,	//	Instantaneous value
			FFC_MAX_VALUE = 1,
			FFC_MIN_VALUE = 2,
			FFC_ERR_VALUE = 3,	//	Value during error state
		};

		/**
		 * data information field
		 */
		struct dif
		{
			std::uint8_t code_ : 4;	//	[0..3] see data_field_code
			std::uint8_t ff_ : 2;	//	[4..5] see function_field_code
			std::uint8_t sn_ : 1;	//	[6] storage number
			std::uint8_t ext_ : 1;	//	[7] extension bit - Specifies if a DIFE Byte follows
		};

		/**
		 * map union over dif bit field
		 */
		union udif
		{
			std::uint8_t raw_;
			dif dif_;
		};

		/**
		 * extension field - up to 10 bytes
		 */
		struct dife
		{
			std::uint8_t sn_ : 4;		//	[0..3] storage number
			std::uint8_t tariff_ : 2;	//	[4..5] tariff 0 = total, 1 = tariff 1, 2 = traiff 2
			std::uint8_t du_ : 1;		//	[6] device unit 0 == reactive, 1, apparent
			std::uint8_t ext_ : 1;		//	[7] extension bit - Specifies if a DIFE Byte follows
		};

		/**
		 * map union over dife bit field
		 */
		union udife
		{
			std::uint8_t raw_;
			dife dife_;
		};

	}
}

#pragma pack(pop)

#endif
