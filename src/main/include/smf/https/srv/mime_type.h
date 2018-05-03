/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */
#ifndef NODE_LIB_HTTPS_SRV_MIME_TYPE_H
#define NODE_LIB_HTTPS_SRV_MIME_TYPE_H

#include <boost/beast/core.hpp>

namespace node
{
	boost::beast::string_view mime_type(boost::beast::string_view path);
}

#endif

