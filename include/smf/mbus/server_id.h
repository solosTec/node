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
	 * Example: 0x3105c => 96072000
	 */
	cyng::buffer_t to_meter_id(std::uint32_t id);

	/*
	 * Example: "10320047" => [10, 32, 00, 47]dec == [0A, 20, 00, 2F]hex
	 */
	cyng::buffer_t to_meter_id(std::string id);

	/**
	 * Produce a string with the format: 
	 * mmmm-nnnnnnnn-vv-uu
	 */
	std::string srv_id_to_str(std::array<char, 9>);
	std::string srv_id_to_str(cyng::buffer_t);


}


#endif	
