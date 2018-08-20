/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_SML_OBIS_PARSER_H
#define NODE_LIB_SML_OBIS_PARSER_H


#include <smf/sml/intrinsics/obis.h>

#include <vector>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>

namespace node	
{
	namespace sml 
	{

		/**
		 *	The string representation of an OBIS is xxx-xxx:xxx.xxx.xxx*xxx where xxx is decimal number
		 *	in the range from 0 up to 255.
		 */
		template <typename InputIterator>
		struct obis_parser
			: boost::spirit::qi::grammar<InputIterator, obis()>
		{
			obis_parser();

			boost::spirit::qi::rule<InputIterator, obis()> r_start;

			boost::spirit::qi::rule<InputIterator, obis()> r_obis_dec;
			boost::spirit::qi::rule<InputIterator, obis()> r_obis_hex;
			boost::spirit::qi::rule<InputIterator, obis()> r_obis_short;
			boost::spirit::qi::rule<InputIterator, obis()> r_obis_medium;
			boost::spirit::qi::int_parser<unsigned short, 16, 2, 2 >	r_byte;
		};


		/**
		 * wrapper function for obis_raw_parser
		 */
		std::pair<obis, bool> parse_obis(std::string const&);


	}
}

#endif
