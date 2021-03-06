/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_LIB_HTTP_SRV_PATH_CAT_H
#define NODE_LIB_HTTP_SRV_PATH_CAT_H

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
