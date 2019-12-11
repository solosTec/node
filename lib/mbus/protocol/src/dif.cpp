/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include <smf/mbus/dif.h>

namespace node
{
	namespace mbus
	{
		data_field_code decode_dif(std::uint8_t dif)
		{
			switch (dif) {
			case DFC_8_BIT_INT:	return DFC_8_BIT_INT;
			case DFC_16_BIT_INT: return DFC_16_BIT_INT;
			case DFC_24_BIT_INT: return DFC_24_BIT_INT;
			case DFC_32_BIT_INT: return DFC_32_BIT_INT;
			case DFC_32_BIT_REAL: return DFC_32_BIT_REAL;
			case DFC_48_BIT_INT: return DFC_48_BIT_INT;
			case DFC_64_BIT_INT: return DFC_64_BIT_INT;
			case DFC_READOUT: return DFC_READOUT;
			case DFC_2_DIGIT_BCD: return DFC_2_DIGIT_BCD;
			case DFC_4_DIGIT_BCD: return DFC_4_DIGIT_BCD;
			case DFC_6_DIGIT_BCD: return DFC_6_DIGIT_BCD;
			case DFC_8_DIGIT_BCD: return DFC_8_DIGIT_BCD;
			case DFC_VAR: return DFC_VAR;
			case DFC_12_DIGIT_BCD: return DFC_12_DIGIT_BCD;
			case DFC_SPECIAL: return DFC_SPECIAL;
			default:
				break;
			}
			return DFC_NO_DATA;
		}
	}
}


