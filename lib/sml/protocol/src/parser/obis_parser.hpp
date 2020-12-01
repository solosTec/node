/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_SML_OBIS_PARSER_HPP
#define NODE_LIB_SML_OBIS_PARSER_HPP

#include <smf/sml/parser/obis_parser.h>
#include <smf/sml/intrinsics/obis_factory.hpp>
#include <boost/spirit/home/support/attributes.hpp>	//	transform_attribute
#include <boost/spirit/include/phoenix.hpp>	//	enable assignment of values like cyy::object

namespace node	
{
	namespace sml
	{
		/**
		 * boost::phoenix::bind cannot distinguish between overloaded function names
		 */
		obis make_obis_phoenix(std::uint32_t a
			, std::uint32_t b
			, std::uint32_t c
			, std::uint32_t d
			, std::uint32_t e
			, std::uint32_t f)
		{
			return make_obis(a, b, c, d, e, f);
		}

		/**
		 * The string representation of an OBIS is xxx-xxx:xxx.xxx.xxx*xxx where xxx is a decimal number
		 * in the range from 0 up to 255. Also a hexadecimal representation is accepted in the form
		 * xx xx xx xx xx xx
		 * The parser also supports a short representation like 0.1.0 or 0.1.2*97. The special case F.F is 
		 * also suppported.
		 */
		template <typename InputIterator>
		obis_parser< InputIterator >::obis_parser()
			: obis_parser::base_type(r_start)
		{
			r_start
				= boost::spirit::qi::lit("F.F")[boost::spirit::qi::_val = boost::phoenix::construct<node::sml::obis>(0, 0, 97, 97, 0, 255)]
				//| boost::spirit::qi::lit("0.0.0")[boost::spirit::qi::_val = boost::phoenix::construct<node::sml::obis>(0, 0, 0, 0, 0, 255)]	//	METER_ADDRESS
				| boost::spirit::qi::lit("F.F.1")[boost::spirit::qi::_val = boost::phoenix::construct<node::sml::obis>(0, 0, 97, 97, 1, 255)]
				| boost::spirit::qi::lit("F.F.2")[boost::spirit::qi::_val = boost::phoenix::construct<node::sml::obis>(0, 0, 97, 97, 2, 255)]
				| boost::spirit::qi::lit("F.F.3")[boost::spirit::qi::_val = boost::phoenix::construct<node::sml::obis>(0, 0, 97, 97, 3, 255)]
				| boost::spirit::qi::lit("F.F.4")[boost::spirit::qi::_val = boost::phoenix::construct<node::sml::obis>(0, 0, 97, 97, 4, 255)]
				| r_obis_medium[boost::spirit::qi::_val = boost::spirit::qi::_1]
				| r_obis_short[boost::spirit::qi::_val = boost::spirit::qi::_1]
				| r_obis_dec[boost::spirit::qi::_val = boost::spirit::qi::_1]
				| r_obis_hex[boost::spirit::qi::_val = boost::spirit::qi::_1]
				;

			//	example: 0.1.2&04
			//	ToDo: set 'manual reset' flag
			//	The '&' signals the values has been reset manually
			r_obis_medium
				= (boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_
					>> '*'
					>> boost::spirit::qi::ushort_)[boost::spirit::qi::_val = boost::phoenix::bind(&make_obis_phoenix, 0, 0, boost::spirit::qi::_1, boost::spirit::qi::_2, boost::spirit::qi::_3, boost::spirit::qi::_4)]
				| (boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_
					>> '&'
					>> boost::spirit::qi::ushort_)[boost::spirit::qi::_val = boost::phoenix::bind(&make_obis_phoenix, 0, 0, boost::spirit::qi::_1, boost::spirit::qi::_2, boost::spirit::qi::_3, boost::spirit::qi::_4)]
				;

			//	example: 0.1.0
			r_obis_short
				= (boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_)[boost::spirit::qi::_val = boost::phoenix::bind(&make_obis_phoenix, 0, 0, boost::spirit::qi::_1, boost::spirit::qi::_2, boost::spirit::qi::_3, 255)]
				| (boost::spirit::qi::lit("C.")	//	Serviceangaben
					>> boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_)[boost::spirit::qi::_val = boost::phoenix::bind(&make_obis_phoenix, 0, 0, 96, boost::spirit::qi::_1, boost::spirit::qi::_2, 255)]
				| (boost::spirit::qi::lit("F.")	//	error messages
					>> boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_)[boost::spirit::qi::_val = boost::phoenix::bind(&make_obis_phoenix, 0, 0, 97, boost::spirit::qi::_1, boost::spirit::qi::_2, 255)]
				| (boost::spirit::qi::lit("L.")	//	Listen
					>> boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_)[boost::spirit::qi::_val = boost::phoenix::bind(&make_obis_phoenix, 0, 0, 98, boost::spirit::qi::_1, boost::spirit::qi::_2, 255)]
				| (boost::spirit::qi::lit("P.")	//	Datenprofile (Lastgang, Betriebslogbuch)
					>> boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_)[boost::spirit::qi::_val = boost::phoenix::bind(&make_obis_phoenix, 0, 0, 99, boost::spirit::qi::_1, boost::spirit::qi::_2, 255)]
				;

			//
			//	accepts: 1-1:1.2.1 and 1-1:1.2.1*255 are the same
			//
			r_obis_dec
				= (boost::spirit::qi::ushort_
					>> '-'
					>> boost::spirit::qi::ushort_
					>> ':'
					>> boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_
					>> '*'
					>> boost::spirit::qi::ushort_)
					[boost::spirit::qi::_val = boost::phoenix::bind(&make_obis_phoenix, boost::spirit::qi::_1, boost::spirit::qi::_2, boost::spirit::qi::_3, boost::spirit::qi::_4, boost::spirit::qi::_5, boost::spirit::qi::_6)]
			| (boost::spirit::qi::ushort_
					>> '-'
					>> boost::spirit::qi::ushort_
					>> ':'
					>> boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_
					>> '.'
					>> boost::spirit::qi::ushort_)
					[boost::spirit::qi::_val = boost::phoenix::bind(&make_obis_phoenix, boost::spirit::qi::_1, boost::spirit::qi::_2, boost::spirit::qi::_3, boost::spirit::qi::_4, boost::spirit::qi::_5, 255)]
				;

			r_obis_hex
				= (r_byte
				>> r_byte
				>> r_byte
				>> r_byte
				>> r_byte
				>> r_byte)[boost::spirit::qi::_val = boost::phoenix::bind(&make_obis_phoenix, boost::spirit::qi::_1, boost::spirit::qi::_2, boost::spirit::qi::_3, boost::spirit::qi::_4, boost::spirit::qi::_5, boost::spirit::qi::_6)]
				;
		}

	}
}

#endif

