/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include "content_parser.hpp"
#include <cyng/util/split.h>
#include <boost/algorithm/string/trim.hpp>

namespace node
{
	namespace http
	{
		template struct content_disposition_parser<std::string::const_iterator>;
		template struct content_disposition_parser<cyng::buffer_t::const_iterator>;

		bool parse_content_disposition(std::string const& inp, content_disposition& result)
		{
			//	Example: form-data; name="file"; filename="Zählpunkte ONEE.CSV"
			auto vec = cyng::split(inp, ";");
			if (!vec.empty()) {
				if (boost::algorithm::contains(vec.at(0), "form-data"))	result.type_ = rfc2183::cdv_form_data;
				else if (boost::algorithm::contains(vec.at(0), "attachment"))	result.type_ = rfc2183::cdv_attachment;
				else result.type_ = rfc2183::cdv_initial;

				for (std::size_t idx = 1; idx < vec.size(); ++idx) {
					auto param = cyng::split(vec.at(idx), "=");
					if (param.size() == 2) {
						boost::algorithm::trim(param.at(0));
						boost::algorithm::trim_if(param.at(1), boost::is_any_of("\""));
						result.params_.emplace_back(param.at(0), param.at(1));
					}
				}
				return true;
			}
			return false;
			//static content_disposition_parser< std::string::const_iterator >	g;
			//std::string::const_iterator first = inp.begin();
			//std::string::const_iterator last = inp.end();
			//return boost::spirit::qi::parse(first, last, g, result);
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
