/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_CRC16_H
#define NODE_SML_CRC16_H


#include <cstdint>
#include <cstddef>	//	std::size_t on windows
#include <smf/sml/defs.h>

#pragma pack(push,1)

namespace node
{
	namespace sml	
	{

		/**
		 * Calculate the initial crc value.
		 *
		 * @return     The initial crc value.
		 */
		static inline std::uint16_t crc_init(void)
		{
			return 0xFFFF;
		}

		/**
		 * Calculate the final crc value.
		 *
		 * @param crc  The current crc value.
		 * @return     The final crc value.
		 */
		static inline std::uint16_t crc_finalize(std::uint16_t crc)
		{
			crc ^= 0xffff;
			crc = ((crc & 0xff) << 8) | ((crc & 0xff00) >> 8);
			return crc;
		}

		/**
		 * Update the crc value with new data.
		 *
		 * @param crc      The current crc value.
		 * @param c        single character
		 * @return         The updated crc value.
		 */
		std::uint16_t crc_update(std::uint16_t crc, unsigned char c);

		/**
		 *	CRC16 FSC implementation based on DIN 62056-46
		 *
		 * @param cp      pointer to array of values
		 * @param len     length of array
		 * @return        The final crc value
		 */
		std::uint16_t sml_crc16_calculate(const unsigned char *cp, int len);
		std::uint16_t sml_crc16_calculate(cyng::buffer_t const&);

		union crc_16_data
		{
			std::uint16_t	crc_;
			std::uint8_t	data_[2];
		};

		/**
		 * calculate the CRC16 value of the buffer and patch this values
		 * on the CRC16 position.
		 */
		std::uint16_t sml_set_crc16(cyng::buffer_t& buffer);

		
	}	//	sml
}	//	node

#pragma pack(pop)

#endif	//	NODE_SML_CRC16_H
