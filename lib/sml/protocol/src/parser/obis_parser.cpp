/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "obis_parser.hpp"

namespace node	
{
	namespace sml
	{

		template struct obis_parser<std::string::const_iterator>;

		std::pair<obis, bool> parse_obis(std::string const& inp)
		{
			obis_parser< std::string::const_iterator >	g;
			std::string::const_iterator first = inp.begin();
			std::string::const_iterator last = inp.end();
			obis result;
			const bool b = boost::spirit::qi::parse(first, last, g, result);
			return std::make_pair(result, b);
		}

	}	
}	//	namespace node
