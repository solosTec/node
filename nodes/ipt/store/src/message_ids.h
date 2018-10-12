/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_MESSAGE_IDS_H
#define NODE_IPT_STORE_MESSAGE_IDS_H

//#include <smf/ipt/bus.h>

namespace node
{
	constexpr std::size_t STORE_EVENT_REGISTER_CONSUMER = 0u;
	constexpr std::size_t STORE_EVENT_REMOVE_CONSUMER = 1u;

	constexpr std::size_t CONSUMER_CREATE_LINE = 0;
	constexpr std::size_t CONSUMER_PUSH_DATA = 1;
	constexpr std::size_t CONSUMER_REMOVE_LINE = 2;
}


#endif
