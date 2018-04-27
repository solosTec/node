/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_CONTENT_DISPOSITION_PARSER_H
#define NODE_LIB_HTTP_CONTENT_DISPOSITION_PARSER_H


#include <smf/http/srv/parser/content_disposition.hpp>
#include <cyng/intrinsics/buffer.h>
//#include <cyx/http/parser/basics_parser.h>

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>

namespace node	
{
	namespace http
	{
		/**
		 * white spaces: SPACE and TAB (no CR and LF)
		 * Doesn't synthesize an attribute
		 */
		template <typename Iterator>
		struct white_space : boost::spirit::qi::grammar< Iterator >
		{
			white_space();
			boost::spirit::qi::rule<Iterator>			r_start;
		};

		//	<CR> <NL> sequence
		template <typename Iterator>
		struct line_separator : boost::spirit::qi::grammar< Iterator >
		{
			line_separator();

			boost::spirit::qi::rule<Iterator>			r_start;
		};

		/**
		 * Quoted (special) characters. Starting with a backslash '\'
		 * Supported values are \n, \r, \v, \b, \t
		 */
		template <typename Iterator>
		struct quoted_pair : boost::spirit::qi::grammar< Iterator, char()>
		{
			quoted_pair();
			boost::spirit::qi::rule<Iterator, char()>	r_start, r_char;
		};

		/**
		 *	Parse a hexadecimal string like %AE as ASCII character
		 */
		template <typename Iterator>
		struct escape_parser : boost::spirit::qi::grammar< Iterator, char() >
		{
			escape_parser();

			boost::spirit::qi::rule<Iterator, char() >	r_start;
			boost::spirit::qi::uint_parser<char, 16, 2, 2>		r_hex;
		};

		/**
		 * All ASCII codes from 0x20 (SPACE) to 0x7e (~)
		 * Synthesize a single character
		 */
		template <typename Iterator>
		struct printable : boost::spirit::qi::grammar< Iterator, char()>
		{
			printable();
			boost::spirit::qi::rule<Iterator, char()>	r_start;
		};

		/**
		 * quoted string (filename="EURO rates";)
		 */
		template <typename Iterator>
		struct quoted_string : boost::spirit::qi::grammar< Iterator, std::string() >
		{
			quoted_string();

			boost::spirit::qi::rule<Iterator, std::string()>	r_start, r_name;
			escape_parser<Iterator>	r_esc_;
		};

		/**	comment
		 *	syntax:
		 * @code
		 comment        = "(" *( ctext | quoted-pair | comment ) ")"
		 ctext          = <any TEXT excluding "(" and ")">
		 * @endcode
		 */
		template <typename Iterator>
		struct comment : boost::spirit::qi::grammar< Iterator, std::string() >
		{
			comment();
			boost::spirit::qi::rule<Iterator, std::string()>	r_start, r_string, r_ctext;
			//boost::spirit::qi::rule<Iterator, char()>	r_ctext;
			printable<Iterator>	r_printable;
			quoted_pair<Iterator>	r_qp;	//	characters starting with backslash '\'
		};

		//	UTF-8 encoded file name
		//	decodes strings starting with utf-8''...
		//	utf-8 encoded file name (filename*=utf-8''%e2%82%ac%20rates)
		template <typename Iterator>
		struct utf8_filename : boost::spirit::qi::grammar< Iterator, std::string() >
		{
			utf8_filename();

			boost::spirit::qi::rule<Iterator, std::string()>	r_start, r_name;
			escape_parser<Iterator>	r_esc_;
		};

		template <typename Iterator>
		struct content_disposition_parser : boost::spirit::qi::grammar<Iterator, content_disposition()>
		{
			content_disposition_parser();

			boost::spirit::qi::rule<Iterator, content_disposition()> r_start;
			boost::spirit::qi::rule<Iterator, param_t()> r_phrase;
			boost::spirit::qi::rule<Iterator, std::string()> r_token, r_value;
			boost::spirit::qi::rule<Iterator> ws_;
			quoted_string<Iterator>	r_quoted_string;
			white_space<Iterator>	r_ws;
			utf8_filename<Iterator>	r_ut8_filename;
		};

		bool parse_content_disposition(std::string const& inp, content_disposition&);
		bool parse_content_disposition(cyng::buffer_t const& inp, content_disposition&);

		template <typename Iterator>
		struct mime_content_type_parser : boost::spirit::qi::grammar<Iterator, mime_content_type()>
		{
			mime_content_type_parser();

			boost::spirit::qi::rule<Iterator, mime_content_type()> content_type_header;
			boost::spirit::qi::rule<Iterator, param_t()> r_phrase;
			boost::spirit::qi::rule<Iterator, std::string()> part, sub_part, r_token, r_attr, value, extension_token;
			boost::spirit::qi::rule<Iterator> ws_;
			line_separator<Iterator>	r_line_sep;
			white_space<Iterator>	r_white_space;
			quoted_pair<Iterator>	r_qp;	//	all quoted symbols like \t
			comment<Iterator>	r_comment;
		};

		bool get_http_mime_type(std::string const& inp, mime_content_type&);
		bool get_http_mime_type(cyng::buffer_t const& inp, mime_content_type&);

	}

}
#endif	
