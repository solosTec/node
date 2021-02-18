/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_HTTP_MIME_TYPE_H
#define SMF_HTTP_MIME_TYPE_H

#include <boost/beast/core.hpp>

namespace smf {
	namespace http {
		boost::beast::string_view mime_type(boost::beast::string_view path);
	}
}

#endif

