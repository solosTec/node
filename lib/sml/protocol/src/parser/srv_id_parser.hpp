/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_SML_SRV_ID_PARSER_HPP
#define NODE_LIB_SML_SRV_ID_PARSER_HPP

#include <smf/sml/parser/srv_id_parser.h>
#include <boost/spirit/home/support/attributes.hpp>	//	transform_attribute
#include <boost/spirit/include/phoenix.hpp>	//	enable assignment of values like cyy::object


namespace node	
{
	namespace sml
	{

		/**
		 * Accept a server ID like 02-e61e-03197715-3c-07
		 * and convert it into a buffer.
		 */
		template <typename InputIterator>
		srv_id_parser< InputIterator >::srv_id_parser()
			: srv_id_parser::base_type(r_start)
		{
			r_start
				= r_long[boost::spirit::qi::_val = boost::spirit::qi::_1]
				| r_short[boost::spirit::qi::_val = boost::spirit::qi::_1]
				;

			r_long
				%= r_hex2
				>> '-'
				>> r_hex2
				>> r_hex2
				>> '-'
				>> r_hex2
				>> r_hex2
				>> r_hex2
				>> r_hex2
				>> '-'
				>> r_hex2
				>> '-'
				>> r_hex2
				;

			r_short
				%= r_hex2
				>> r_hex2
				>> r_hex2
				>> r_hex2
				;

		}

	}
}

#endif

