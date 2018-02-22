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
		 *	@param id manufacturer id (contains 3 characters)
		 *	@param code encoded manufacturer id (contains 2 bytes)
		 *	@return true if encoding was successful
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
}	//	node


#endif	
