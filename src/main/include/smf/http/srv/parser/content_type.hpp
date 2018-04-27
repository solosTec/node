/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTP_CONTENT_TYPE_HPP
#define NODE_LIB_HTTP_CONTENT_TYPE_HPP

//#include <cyx/http/http.h>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>
#include <boost/algorithm/string/predicate.hpp>

namespace node	
{
	namespace http
	{
		//	parameter types
		using param_t = std::pair<std::string, std::string>;
		using param_container_t = std::vector < param_t >;

		inline std::string lookup_param(param_container_t const& phrases, std::string const& name)
		{
			auto pos = std::find_if(phrases.begin(), phrases.end(), [&name](param_t const& p)
			{
				return (boost::algorithm::iequals(p.first, name));
			});

			return (pos != phrases.end())
				? pos->second
				: ""
				;
		}

		//	MIME Content-Type

		/**
		 * @see http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
		 *
		 * example:
		 * @code
		 * Content-Type: text/html; charset=ISO-8859-4
		 * @endcode
		 */
		struct mime_content_type
		{
			std::string			type_;
			std::string 		sub_type_;
			param_container_t	phrases_;

			mime_content_type()
				: type_()
				, sub_type_()
				, phrases_()
			{}

			mime_content_type(std::string const& type
				, std::string const& subtype
				, param_container_t const& phrases)
				: type_(type)
				, sub_type_(subtype)
				, phrases_(phrases)
			{}
		};

		inline mime_content_type make_mime_type(std::string const& type, std::string const& subtype)
		{
			const param_container_t tmp;
			return mime_content_type(type, subtype, tmp);
		}

		inline bool is_matching(mime_content_type const& mct
			, std::string const& type
			, std::string const& subtype)
		{

			return boost::algorithm::iequals(mct.type_, type)
				&& boost::algorithm::iequals(mct.sub_type_, subtype)
				;

		}

		inline std::string to_str(mime_content_type const& mct)
		{

			//	serialize content type
			std::stringstream ss;
			ss
				<< mct.type_
				<< '/'
				<< mct.sub_type_
				;

			std::for_each(mct.phrases_.begin(), mct.phrases_.end(), [&ss](param_t const& p) {
				ss
					<< ';'
					<< p.first
					<< '='
					<< p.second
					;
			});

			return ss.str();
		}

		/*
		 *	The Content-Type field for multipart entities requires one parameter,
		 *	"boundary", which is used to specify the encapsulation boundary.
		 *	The encapsulation boundary is defined as a line consisting entirely
		 *	of two hyphen characters ("-", decimal code 45) followed by the boundary
		 *	parameter value from the Content-Type header field.
		 */
		inline std::string get_boundary(param_container_t const& phrases)
		{
			return std::string("--") + lookup_param(phrases, "boundary");
		}
	}
}

#endif