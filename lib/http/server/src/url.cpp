/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Sylko Olzscher 
 * 
 */ 

#include <smf/http/srv/url.h>

namespace node 
{
	std::string decode_url(std::string const& url)
	{
		enum
		{
			SEARCH = 0, ///< searching for an ampersand to convert
			DECODE_1,		///< convert the two proceeding characters from hex
			DECODE_2,		///< convert the two proceeding characters from hex
		} state = SEARCH;


		std::string result;
		result.reserve(url.size());

		char c{ 0 };

		for (auto idx = 0; idx < url.size(); ++idx) {
			switch (state) {

			case SEARCH:
				if (url.at(idx) == '%' && (idx < url.size())) {
					state = DECODE_1;
				}
				else {
					result += url.at(idx);
				}
				break;

			case DECODE_1:
				if (url.at(idx) > 0x2F && url.at(idx) < 0x3A) {
					c = char(url.at(idx) - 0x30) << 4;
					state = DECODE_2;
				}
				else {
					result += '%';
					result += url.at(idx);
				}
				break;

			case DECODE_2:
				if (url.at(idx) > 0x2F && url.at(idx) < 0x3A) {
					c += char(url.at(idx) - 0x30);
					result += c;
				}
				else {
					result += '%';
					result += c;
					result += url.at(idx);
				}
				state = SEARCH;
				break;
			}
		}

		return result;
	}
}
