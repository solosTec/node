/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_DIF_H
#define SMF_MBUS_DIF_H

#include <cstdint>
#include <cstddef>

namespace smf
{
	namespace mbus	
	{
		enum class data_field_code : std::uint8_t {
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
		 * Some data types have a variable length!
		 * 
		 * @return fixed length in bytes
		 */
		constexpr std::size_t get_length(data_field_code dfc) {
			switch (dfc) {
			case data_field_code::DFC_NO_DATA:		return 0;
			case data_field_code::DFC_8_BIT_INT:	return 1;	//	8 Bit Integer
			case data_field_code::DFC_16_BIT_INT:	return 2;	//	16 Bit Integer
			case data_field_code::DFC_24_BIT_INT:	return 3;	//	24 Bit Integer
			case data_field_code::DFC_32_BIT_INT:	return 4;	//	32 Bit Integer
			case data_field_code::DFC_32_BIT_REAL:	return 4;	//	32 Bit Real
			case data_field_code::DFC_48_BIT_INT:	return 6;	//	48 Bit Integer
			case data_field_code::DFC_64_BIT_INT:	return 8;	//	64 Bit Integer
			case data_field_code::DFC_READOUT:		return 0xff;	//	Selection for Readout
			case data_field_code::DFC_2_DIGIT_BCD:	return 1;	//	2 digit BCD
			case data_field_code::DFC_4_DIGIT_BCD:	return 2;	// 	4 digit BCD
			case data_field_code::DFC_6_DIGIT_BCD:	return 3;	//	5 digit BCD
			case data_field_code::DFC_8_DIGIT_BCD:	return 4;	//	8 digit BCD
			case data_field_code::DFC_VAR:			return 0xff;	// variable length - The length of the data is given with the first byte of data, which is here called LVAR.
			case data_field_code::DFC_12_DIGIT_BCD:	return 6;	// 12 digit BCD
			case data_field_code::DFC_SPECIAL:		return 0xff;	// special functions
			default:
				break;
			}
			return 0;
		}
		/**
		 * Take a DIF value and returns the enum.
		 */
		constexpr data_field_code decode_dfc(std::uint8_t val) {
			return static_cast<data_field_code>(val & 0x0f);
		}

		enum class function_field_code : std::uint8_t {
			INSTANT = 0,	//	Instantaneous value
			MAX_VALUE = 1,
			MIN_VALUE = 2,
			ERR_VALUE = 3,	//	Value during error state
		};

		/**
		 * Take a DIF value and returns the enum.
		 */
		constexpr function_field_code decode_ffc(std::uint8_t val) {
			return static_cast<function_field_code>((val & 0x30) >> 4);
		}

		class dif
		{
		public:
			explicit dif(char);
			explicit dif(std::uint8_t);
			dif() = delete;

			/**
			 * bit [0..3] see data_field_code
			 */
			data_field_code get_data_field_code() const;

			/**
			 * bit [4..5] see function_field_code
			 */
			function_field_code get_function_field_code() const;

			/**
			 * bit [6]
			 */
			bool is_storage() const;

			/**
			 * bit [7]
			 */
			bool is_extended() const;

		private:
			std::uint8_t value_;
		};

		class dife
		{
		public:
			explicit dife(char);
			explicit dife(std::uint8_t);
			dife() = delete;

			/**
			 * bit [0..3] storage number
			 */
			std::uint8_t get_storage_nr() const;
			/**
			 * bit [4..5] tariff 0 = total, 1 = tariff 1, 2 = traiff 2
			 */
			std::uint8_t get_tariff() const;

			/**
			 * bit [7]
			 */
			bool is_extended() const;

		private:
			std::uint8_t value_;
		};
	}
}


#endif
