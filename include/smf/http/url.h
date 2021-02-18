/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_HTTP_URL_H
#define SMF_HTTP_URL_H

#include <string>

namespace smf {
	namespace http {
		std::string decode_url(std::string const&);
	}
}

#endif
