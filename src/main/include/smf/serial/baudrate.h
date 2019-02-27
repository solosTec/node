/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#ifndef NODE_SERIAL_BAUDRATE_H
#define NODE_SERIAL_BAUDRATE_H


#include <cstdint>
#include <limits>

namespace node
{
	namespace serial	
	{
		enum baudrate : std::uint32_t
		{
			BR_50 = 1u,
			BR_75 = 2u,
			BR_110 = 3u,
			BR_134 = 4u,
			BR_150 = 5u,
			BR_200 = 6u,
			BR_300 = 7u,

			BR_600 = 10u,
			BR_1200 = 11u,
			BR_1800 = 12u,
			BR_2400 = 13u,
			BR_4800 = 14u,
			BR_9600 = 15u,
			BR_19200 = 16u,
			BR_38400 = 17u,

			BR_57600 = 1001u,
			BR_115200 = 10002u,
			BR_230400 = 10003u,

			BR_460800 = 10004u,
			BR_500000 = 10005u,
			BR_576000 = 10006u,
			BR_921600 = 10007u,

			BR_1000000 = 100010u,
			BR_1152000 = 100012u,
			BR_1500000 = 100013u,
			BR_2000000 = 100014u,
			BR_2500000 = 100015u,
			BR_3000000 = 100016u,
			BR_3500000 = 100017u,
			BR_4000000 = 100018u,

			BR_INVALID = std::numeric_limits<std::uint32_t>::max()
		};

		/**
		 * Define an array of valid baudrates
		 */
		constexpr std::uint32_t baudrates[] = {

			50u,
			75u,
			110u,
			134u,
			150u,
			200u,
			300u,

			600u,
			1200u,
			1800u,
			2400u,
			4800u,
			9600u,
			19200u,
			38400u,

			57600u,
			115200u,
			230400u,

			460800u,
			500000u,
			576000u,
			921600u,

			1000000u,
			1152000u,
			1500000u,
			2000000u,
			2500000u,
			3000000u,
			3500000u,
			4000000u,

			std::numeric_limits<std::uint32_t>::max()
		};

		/**
		 * @return B/sec for the specified baudrate
		 */
		std::uint32_t get_speed(baudrate);

		/**
		 * @return the baudrate code for the specified speed
		 */
		baudrate get_baudrate(std::uint32_t);

		/**
		 * adjust the value to a valid baudrate by implementing
		 * a nearest neighbor search.
		 */
		std::uint32_t adjust_baudrate(std::uint32_t);
	}
}	//	node


#endif	//	NODE_SERIAL_BAUDRATE_H
