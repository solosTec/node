/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_CONTENT_DISPOSITION_PARSER_HPP
#define NODE_LIB_HTTP_CONTENT_DISPOSITION_PARSER_HPP

#include <smf/http/srv/parser/content_parser.h>

#include <boost/fusion/include/std_pair.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/fusion/adapted/struct.hpp>

namespace node	
{
	namespace http
	{
		namespace
		{	//	*anonymous*
			struct cdv_parser
				: boost::spirit::qi::symbols<char, rfc2183::values>
			{
				cdv_parser()
				{
					/**
					*	Only lower case entries are valid. See remark below.
					*/
					add
					("inline", rfc2183::cdv_inline)
						("attachment", rfc2183::cdv_attachment)
						("form-data", rfc2183::cdv_form_data)
						("signal", rfc2183::cdv_signal)
						("alert", rfc2183::cdv_alert)
						("icon", rfc2183::cdv_icon)
						("render", rfc2183::cdv_render)
						("recipient-list-history", rfc2183::cdv_recipient_list_history)
						("session", rfc2183::cdv_session)
						("aib", rfc2183::cdv_aib)
						("early-session", rfc2183::cdv_early_session)
						("recipient-list", rfc2183::cdv_recipient_list)
						("notification", rfc2183::cdv_notification)
						("by-reference", rfc2183::cdv_by_reference)
						("info-package", rfc2183::cdv_info_package)
						("recording-session", rfc2183::cdv_recording_session)
						;

				}
			}	content_disposition_values;
		}	//	*anonymous*

		template <typename Iterator>
		content_disposition_parser< Iterator > ::content_disposition_parser()
			: content_disposition_parser::base_type(r_start)
		{
			r_start
				= *r_ws
				>> content_disposition_values
				>> *r_phrase
				;

			r_phrase
				%= boost::spirit::qi::lit(';')
				>> +r_ws
				>> r_token	//.alias()
				>> '='
				>> r_value
				>> *r_ws
				;

			r_token
				= +(boost::spirit::qi::alnum | boost::spirit::qi::char_("-*"))
				;

			r_value
				= r_ut8_filename	//	utf-8''%e2%82%ac%20rates
				| r_quoted_string	//	"demo.jpg"
				| r_token
				;

		}

		template <typename Iterator>
		quoted_string<Iterator>::quoted_string()
			: quoted_string::base_type(r_start)
		{
			//	quoted string(filename = "EURO rates";)

			r_start
				= boost::spirit::qi::lexeme['"' >> r_name >> '"'];
			;

			r_name
				= *(r_esc_ | (boost::spirit::ascii::char_ - '"'))
				;

		}

		template <typename Iterator>
		escape_parser<Iterator>::escape_parser()
			: escape_parser::base_type(r_start)
		{
			//  this statement requires to include the phoenix library
			r_start
				= ('%' >> r_hex)[boost::spirit::_val = boost::spirit::_1]
				;
		}

		template <typename Iterator>
		white_space<Iterator>::white_space()
			: white_space::base_type(r_start)
		{
			//	same as boost::spirit::ascii::blank
			r_start
				= boost::spirit::qi::lit(' ')
				| boost::spirit::qi::lit('\t')
				;
		}

		template <typename Iterator>
		line_separator<Iterator>::line_separator()
			: line_separator::base_type(r_start)
		{

			r_start
				= boost::spirit::qi::lexeme[boost::spirit::qi::lit("\015\012")]
				;
		}

		namespace
		{	//	*anonymous*
			struct esc_symbols : boost::spirit::qi::symbols<char const, char const>
			{
				/**
				* same escape symbols as in C/C++
				* HTTP supports some addiotional characters.
				*/
				esc_symbols()
				{
					add
						//	C/C++
						("\\a", '\a')
						("\\b", '\b')
						("\\f", '\f')
						("\\n", '\n')
						("\\r", '\r')
						("\\t", '\t')
						("\\v", '\v')
						("\\\\", '\\')
						("\\\'", '\'')
						("\\\"", '\"')

						//	rfc822
						("\\(", '(')
						("\\)", ')')
						("\\<", '<')
						("\\>", '>')
						("\\@", '@')
						("\\;", ';')
						("\\,", ',')
						("\\.", '.')
						("\\[", '[')
						("\\]", ']')
						;
				}
			}	r_quoted;
		}	//	*anonymous*

		template <typename Iterator>
		quoted_pair<Iterator>::quoted_pair()
			: quoted_pair::base_type(r_start)
		{
			r_start
				= r_quoted
				;
		}

		template <typename Iterator>
		utf8_filename<Iterator>::utf8_filename()
			: utf8_filename::base_type(r_start)
		{
			//	utf-8 encoded file name (filename*=utf-8''%e2%82%ac%20rates)

			r_start
				= boost::spirit::qi::lit("utf-8''")
				>> r_name
				;

			r_name
				= +(boost::spirit::qi::alnum | r_esc_)
				;

		}

		template <typename Iterator>
		comment<Iterator>::comment()
			: comment::base_type(r_start)
		{
			r_start
				= boost::spirit::qi::lit('(') >> r_string >> boost::spirit::qi::lit(')')
				;

			r_string
				= boost::spirit::qi::lit('(') >> r_string.alias() >> boost::spirit::qi::lit(')')
				| r_ctext
				//| *r_qp
				;

			//	any printable character except '(' and ')'
			r_ctext
				= boost::spirit::qi::hold[*(r_printable - boost::spirit::qi::char_("()"))]
				;
		}

		template <typename Iterator>
		printable<Iterator>::printable()
			: printable::base_type(r_start)
		{

			r_start
				= boost::spirit::qi::char_(" -~")
				;
		}


		//	examples:
		//	multipart/form-data; boundary=----WebKitFormBoundaryBStJolBqHji2FoKK
		template <typename Iterator>
		mime_content_type_parser< Iterator > ::mime_content_type_parser()
			: mime_content_type_parser::base_type(content_type_header)
		{
			content_type_header
				= *boost::spirit::qi::lit(' ')
				>> part
				>> '/'
				>> sub_part
				>> *r_phrase
				;

			part
				= r_token
				| extension_token
				;

			sub_part
				= r_token
				| extension_token
				;

			r_phrase
				= boost::spirit::qi::lit(';')
				>> +ws_
				>> r_attr
				>> '='
				>> value
				>> *ws_
				;

			ws_
				= r_white_space
				| r_line_sep
				| r_comment
				;

			r_attr = r_token.alias();

			value
				= +(r_qp | boost::spirit::qi::char_(" -~"))
				;

			r_token
				= +(boost::spirit::qi::alnum | boost::spirit::qi::char_("-"))
				;

			extension_token
				= boost::spirit::qi::char_("Xx")
				>> boost::spirit::qi::lit('-')
				>> r_token
				;
		}

	}
}

BOOST_FUSION_ADAPT_STRUCT(
	node::http::content_disposition,
	(node::http::rfc2183::values, type_)
	(node::http::param_container_t, params_)
	)

BOOST_FUSION_ADAPT_STRUCT(
	node::http::mime_content_type,
	(std::string, type_)
	(std::string, sub_type_)
	(node::http::param_container_t, phrases_)
)

#endif
