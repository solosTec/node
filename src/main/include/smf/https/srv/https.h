/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_HTTPS_H
#define NODE_LIB_HTTPS_H

#include <cyng/log.h>
#include <cyng/intrinsics/sets.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/uuid/uuid.hpp>

namespace node
{
	namespace https
	{
		using server_callback_t = std::function<void(boost::uuids::uuid, cyng::vector_t&&)>;
		using session_callback_t = std::function<void(cyng::vector_t&&)>;
	}
}

#endif