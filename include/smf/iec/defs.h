/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IEC_DEFS_H
#define SMF_IEC_DEFS_H

#include <cstdint>

namespace smf
{
	namespace iec
	{
		constexpr std::uint8_t SOH = 0x01;	//	start of header
		constexpr std::uint8_t STX = 0x02;	//	start data frame
		constexpr std::uint8_t ETX = 0x03;	//	end data frame
		constexpr std::uint8_t EOT = 0x04;	//	end of partial block
		constexpr std::uint8_t ACK = 0x06;	//	acknownledge
		constexpr std::uint8_t NAK = 0x15;	//	negative acknowledge
		constexpr std::uint8_t  CR = 0x0D;	//	carriage return
		constexpr std::uint8_t  LF = 0x0A;	//	line feed

		/** @brief more special characters
		 *
		 * / - start character
		 * ! - end character
		 * ? - Transmission request command
		 */
		constexpr char start_c = '/';
		constexpr char end_c = '!';
		constexpr char transmission_req_c = '?';

		 /** @brief Reduced ID codes (e.g. for IEC 62056-21)
		  *
		  * A-B:C.D.E*F automatic reset
		  * A-B:C.D.E&F manual reset
		  *
		  * F.F(nnnnnnnn) contains the error flag
		  *
		  * @see http://dlms.com/documents/Excerpt_BB11.pdf chapter 7.11.1
		  */

		  //
		  //	Value group C 
		  //
		constexpr std::uint8_t _C = 96;	//	General service entries (C)
		constexpr std::uint8_t _F = 97;	//	General error messages (F)
		constexpr std::uint8_t _L = 98;	//	General list objects (L)
		constexpr std::uint8_t _P = 99;	//	Abstract data profiles (P)

	}
}

#endif
