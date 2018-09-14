/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_MBUS_DEFS_H
#define NODE_SML_MBUS_DEFS_H

#include <string>
#include <cstdint>

namespace node
{
	namespace sml	
	{

		/**
		 *	@param id manufacturer id (contains 3 uppercase characters)
		 *	@return manufacturer id or 0 in case of error
		 *	@see http://www.dlms.com/flag/
		 */
		std::uint16_t encode_id(std::string const& id);

		/**
		 *	@param code encoded manufacturer id (contains 2 bytes)
		 *	@return string with manufacturer ID (consisting of 3 characters)
		 */
		std::string decode(std::uint16_t const&);
		std::string decode(char, char);

		/**
		 * Read the manufacturer name from a hard coded table
		 * 
		 *	@param code encoded manufacturer id (contains 2 bytes)
		 *	@return string with manufacturer name
		 */
		std::string get_manufacturer_name(std::uint16_t const&);
		std::string get_manufacturer_name(char, char);		
		
		/**
		 * @return a the medium name (in english)
		 * @see http://www.m-bus.com/mbusdoc/md8.php
		 */
		std::string get_medium_name(std::uint8_t m);

	}	//	sml

	namespace mbus
	{
		//
		//	data link layer
		//

		//
		//	max frame size is 252 bytes
		//
		constexpr std::uint8_t FRAME_DATA_LENGTH = 0xFC;

		//
		// Frame start/stop bits
		//
		constexpr std::uint8_t FRAME_ACK_START	= 0xE5;	//	acknowledge receipt of transmissions
		constexpr std::uint8_t FRAME_SHORT_START = 0x10;
		constexpr std::uint8_t FRAME_CONTROL_START = 0x68;
		constexpr std::uint8_t FRAME_LONG_START = 0x68;
		constexpr std::uint8_t FRAME_STOP = 0x16;

		//
		//	packet types
		//
		constexpr std::uint8_t CTRL_FIELD_SP_UD = 0x08;	//	
		constexpr std::uint8_t CTRL_FIELD_SND_NKE = 0x40;	//	
		constexpr std::uint8_t CTRL_FIELD_SND_NR = 0x44;	//	Send spontaneous/periodical application data without request  (S1 mode)
		constexpr std::uint8_t CTRL_FIELD_REQ_UD1 = 0x5A;	//	Request User Data
		constexpr std::uint8_t CTRL_FIELD_REQ_UD2 = 0x5B;	//	Request User Data
		constexpr std::uint8_t CTRL_FIELD_SND_UD = 0x53;	//	
		//constexpr std::uint8_t CTRL_FIELD_IR = 0x00;		//	Send manually initiated installation data – Send installation request
		//constexpr std::uint8_t CTRL_FIELD_ACC_NR = 0x00;	//	No data – Provides opportunity to access the meter between two application transmissions
		//constexpr std::uint8_t CTRL_FIELD_ACC_DMD = 0x00;	//	Access demand to master in order to request new importat application data – Alerts
		//constexpr std::uint8_t CTRL_FIELD_ACC = 0x00;		//	Acknowledge the reception 
		//constexpr std::uint8_t CTRL_FIELD_RSP_UD = 0x00;	//	Response of application data after a request from master
	}

}	//	node


#endif	
