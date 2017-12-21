/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2017 Sylko Olzscher 
 * 
 */ 
#ifndef NODE_HTTP_MIME_TYPE_H
#define NODE_HTTP_MIME_TYPE_H

#include <boost/beast/core.hpp>

namespace node 
{
	boost::beast::string_view mime_type(boost::beast::string_view path);
}

#endif

