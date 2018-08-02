/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_IEC_DEFS_H
#define NODE_SML_IEC_DEFS_H

#include <string>
#include <cstdint>

namespace node
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

		/** @brief Reduced ID codes (e.g. for IEC 62056-21) 
		 * 
		 * A-B:C.D.E*F automatic reset
		 * A-B:C.D.E&F manual reset
		 *
		 * F.F(nnnnnnnn) contains the error flag
		 *
		 * @see http://dlms.com/documents/Excerpt_BB11.pdf chapter 7.11.1
		 */

	}	//	iec


}	//	node


#endif	
