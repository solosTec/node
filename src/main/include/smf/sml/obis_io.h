/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_OBIS_IO_H
#define NODE_SML_OBIS_IO_H

#include <smf/sml/intrinsics/obis.h>
#include <ostream>

namespace node
{
	namespace sml
	{
		/**
		 * Decimal formatting: nnn-nnn:nnn.nnn.nnn*nnn 
		 */
		std::ostream& operator<<(std::ostream& os, obis const&);
		
		/**
		 * Maximum length is 23 characters
		 * 
		 * @return OBIS code as decimal string with the format:
		 * nnn-nnn:nnn.nnn.nnn*nnn
		 */
		std::string to_string(obis const&);

		/**
		 * The values are separated by spaces. The resulting string 
		 * has a fixed length of 17 bytes.
		 *
		 * @return OBIS code in hex format:
		 * xx xx xx xx xx xx
		 */
		std::string to_hex(obis const&);
		std::ostream& to_hex(std::ostream&, obis const&);

		/**
		 * Try to convert a textual OBIS representation into an valid
		 * OBIS code. Accepts decimal and hexadescimal representations.
		 * Returns a NULL OBIS code if conversion failed.
		 */
		obis to_obis(std::string const&);

		/**
		 * The OBIS codes are separated by a single separator.
		 *
		 * @return OBIS code path:
		 * xx xx xx xx xx xx  xx xx xx xx xx xx
		 */
		std::ostream& to_hex(std::ostream&, obis_path_t const&, char sep);

		/**
		 * Convert OBIS codes into hexadescimal representation
		 * separated by a single character (sep).
		 */
		std::string to_hex(obis_path_t const&, char sep);

		/**
		 * Try to convert a textual OBIS path representation into an valid
		 * obis_path_t. Accepts hexadeximal and decimal formats.
		 */
		obis_path_t to_obis_path(std::string const&, char sep);

		/**
		 * @param translate if true all known OBIS codes will be substituted to
		 * human readable form.
		 * @return vector of strings
		 */
		std::vector<std::string> transform_to_str_vector(obis_path_t const&, bool translate);
		std::ostream& transform_to_str(std::ostream& os, obis_path_t const&, bool translate, char sep);
		std::string transform_to_str(obis_path_t const&, bool translate, char sep);

		/**
		 * @param translate if true all known OBIS codes will be substituted to
		 * human readable form.
		 * @return vector of objects of type string
		 */
		cyng::vector_t transform_to_obj_vector(obis_path_t const&, bool translate);

	}	//	sml
}	//	node


#endif	
