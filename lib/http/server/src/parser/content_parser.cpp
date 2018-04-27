/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "content_parser.hpp"
//#include <cyng/intrinsics/buffer.h>

namespace node
{
	namespace http
	{
		template struct content_disposition_parser<std::string::const_iterator>;
		template struct content_disposition_parser<cyng::buffer_t::const_iterator>;

		bool parse_content_disposition(std::string const& inp, content_disposition& result)
		{
			static content_disposition_parser< std::string::const_iterator >	g;
			std::string::const_iterator first = inp.begin();
			std::string::const_iterator last = inp.end();
			return boost::spirit::qi::parse(first, last, g, result);
		}

		bool parse_content_disposition(cyng::buffer_t const& inp, content_disposition& result)
		{
			static content_disposition_parser< cyng::buffer_t::const_iterator >	g;
			cyng::buffer_t::const_iterator first = inp.begin();
			cyng::buffer_t::const_iterator last = inp.end();
			return boost::spirit::qi::parse(first, last, g, result);
		}

		template struct mime_content_type_parser<std::string::const_iterator>;
		template struct mime_content_type_parser<cyng::buffer_t::const_iterator>;

		bool get_http_mime_type(std::string const& inp, mime_content_type& result)
		{

			static mime_content_type_parser< std::string::const_iterator >	g;
			std::string::const_iterator first = inp.begin();
			std::string::const_iterator last = inp.end();
			return boost::spirit::qi::parse(first, last, g, result);
		}

		bool get_http_mime_type(cyng::buffer_t const& inp, mime_content_type& result)
		{

			static mime_content_type_parser< cyng::buffer_t::const_iterator >	g;
			cyng::buffer_t::const_iterator first = inp.begin();
			cyng::buffer_t::const_iterator last = inp.end();
			return boost::spirit::qi::parse(first, last, g, result);
		}

	}
}
