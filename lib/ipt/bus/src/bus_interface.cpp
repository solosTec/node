/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/ipt/bus_interface.h>

namespace node
{
	namespace ipt
	{
		std::uint64_t build_line(std::uint32_t channel, std::uint32_t source)
		{
			//
			//	create the line ID by combining source and channel into one 64 bit integer
			//
			return (((std::uint64_t)channel) << 32) | ((std::uint64_t)source);
		}

	}
}
