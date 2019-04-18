/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_SML_SRV_ID_PARSER_H
#define NODE_LIB_SML_SRV_ID_PARSER_H


#include <cyng/intrinsics/buffer.h>

#include <vector>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>

namespace node	
{
	namespace sml 
	{

		/** @brief parser for server IDs
		 *
		 * Accept a server ID like 02-e61e-03197715-3c-07
		 * and convert it into a buffer.
		 */
		template <typename InputIterator>
		struct srv_id_parser
			: boost::spirit::qi::grammar<InputIterator, cyng::buffer_t()>
		{
			srv_id_parser();

			boost::spirit::qi::rule<InputIterator, cyng::buffer_t()>	r_start, r_long, r_medium, r_short, r_mac;
			boost::spirit::qi::uint_parser<unsigned int, 16, 2, 2>		r_hex2;
		};


		/**
		 * Accept a server ID like 02-e61e-03197715-3c-07
		 * and convert it into a buffer.
		 */
		std::pair<cyng::buffer_t, bool> parse_srv_id(std::string const&);
	}
}

#endif
