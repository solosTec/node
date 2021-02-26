/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_MBUS_SERVER_ID_H
#define SMF_MBUS_SERVER_ID_H

#include <cyng/obj/intrinsics/buffer.h>

#include <array>
#include <cstdint>

namespace smf
{
	/**
	 * define an array that contains the server ID
	 */
	using srv_id_t = std::array<std::uint8_t, 9>;

	/*
	 * Example: 0x3105c = > 96072000
	 */
	cyng::buffer_t to_meter_id(std::uint32_t id);

}


#endif	
