/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_FLAG_ID_H
#define SMF_MBUS_FLAG_ID_H

#include <string>
#include <cstdint>

namespace smf
{
	namespace mbus	
	{

		/**
		 *	@param id manufacturer id (contains 3 uppercase characters)
		 *	@return manufacturer id or 0 in case of error
		 *	@see https://www.dlms.com/flag-id/flag-id-list
		 */
		std::uint16_t encode_id(std::string const& id);

		/**
		 *	@param code encoded manufacturer id (contains 2 bytes)
		 *	@return string with manufacturer ID (consisting of 3 characters)
		 */
		std::string decode(std::uint16_t);
		std::string decode(char, char);

		/**
		 * Read the manufacturer name from a hard coded table
		 * 
		 *	@param code encoded manufacturer id (contains 2 bytes)
		 *	@return string with manufacturer name
		 */
		std::string get_manufacturer_name(std::uint16_t&);
		std::string get_manufacturer_name(char, char);		
		

	}
}


#endif	
