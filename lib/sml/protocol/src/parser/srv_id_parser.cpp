/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include <parser/srv_id_parser.hpp>

namespace node	
{
	namespace sml
	{
		template struct srv_id_parser<std::string::const_iterator>;

		std::pair<cyng::buffer_t, bool> parse_srv_id(std::string const& inp)
		{
			srv_id_parser< std::string::const_iterator >	g;
			std::string::const_iterator first = inp.begin();
			std::string::const_iterator last = inp.end();
			cyng::buffer_t result;
			const bool b = boost::spirit::qi::parse(first, last, g, result);
			return std::make_pair(result, b);
		}

	}	
}	//	namespace node
