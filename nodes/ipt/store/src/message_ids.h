/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_STORE_MESSAGE_IDS_H
#define NODE_IPT_STORE_MESSAGE_IDS_H

namespace node
{
	constexpr std::size_t STORE_EVENT_REGISTER_CONSUMER = 0u;
	constexpr std::size_t STORE_EVENT_REMOVE_PROCESSOR = 1u;

	constexpr std::size_t CONSUMER_CREATE_LINE = 0;
	constexpr std::size_t CONSUMER_PUSH_DATA = 1;
	constexpr std::size_t CONSUMER_REMOVE_LINE = 2;
	constexpr std::size_t CONSUMER_EOM = 3;
}


#endif
