/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Sylko Olzscher
 *
 */

#include <smf/serial/baudrate.h>

namespace node
{
	namespace serial
	{
		std::uint32_t get_value(baudrate br)
		{
			switch (br) {
			case BR_50:	return 50u;
			case BR_75:	return 75u;
			case BR_110:	return 110u;
			case BR_134:	return 134u;
			case BR_150:	return 150u;
			case BR_200:	return 200u;
			case BR_300:	return 300u;

			case BR_600:	return 600u;
			case BR_1200:	return 1200u;
			case BR_1800:	return 1800u;
			case BR_2400:	return 2400u;
			case BR_4800:	return 4800u;
			case BR_9600:	return 9600u;
			case BR_19200:	return 1920u;
			case BR_38400:	return 38400u;

			case BR_57600:	return 57600u;
			case BR_115200:	return 115200u;
			case BR_230400:	return 230400u;

			case BR_460800:	return 460800u;
			case BR_500000:	return 500000u;
			case BR_576000:	return 576000u;
			case BR_921600:	return 921600u;

			case BR_1000000:	return 1000000u;
			case BR_1152000:	return 1152000u;
			case BR_1500000:	return 1500000u;
			case BR_2000000:	return 2000000u;
			case BR_2500000:	return 2500000u;
			case BR_3000000:	return 3000000u;
			case BR_3500000:	return 3500000u;
			case BR_4000000:	return 4000000u;

			default:
				break;
			}
			return std::numeric_limits<std::uint32_t>::max();
		}

		std::uint32_t adjust_baudrate(std::uint32_t v)
		{
			//
			//	precondition: lowest and greatest values
			//
			if (v < 50u)	return 50u;
			if (v > 4000000u)	return 4000000u;

			std::uint32_t speed = v;
			std::int32_t dist = std::numeric_limits<std::int32_t>::max();
			for (auto const br : baudrates) {

				//
				//	calculate distance and adjust
				//
				if (v > br) {
					if ((v - br) < dist) {
						dist = v - br;
						speed = br;
					}
					else {
						break;
					}
				}
				else if (v < br) {
					if ((br - v) < dist) {
						dist = br - v;
						speed = br;
					}
					else {
						break;
					}
				}
				else {
					//
					//	stop condition: perfect match
					//
					speed = br;
					break;
				}
			}
			return speed;
		}

	}
}
