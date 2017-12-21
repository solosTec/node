/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 

#include "path_cat.h"

namespace node 
{
	std::string path_cat(
		boost::beast::string_view base,
		boost::beast::string_view path)
	{
		if(base.empty())
			return path.to_string();
		std::string result = base.to_string();
#if BOOST_MSVC
		char constexpr path_separator = '\\';
		if(result.back() == path_separator)
			result.resize(result.size() - 1);
		result.append(path.data(), path.size());
		for(auto& c : result)
			if(c == '/')
				c = path_separator;
#else
		char constexpr path_separator = '/';
		if(result.back() == path_separator)
			result.resize(result.size() - 1);
		result.append(path.data(), path.size());
#endif
		return result;
	}
}
