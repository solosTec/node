/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HTTP_PATH_CAT_H
#define NODE_HTTP_PATH_CAT_H

#include <boost/beast/core.hpp>

namespace node 
{
	// Append an HTTP rel-path to a local filesystem path.
	// The returned path is normalized for the platform.
	std::string path_cat(
		boost::beast::string_view base,
		boost::beast::string_view path);
}

#endif
