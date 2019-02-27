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
		std::uint32_t get_speed(baudrate br)
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

		baudrate get_baudrate(std::uint32_t speed)
		{
			switch (speed) {
			case 50u:	return BR_50;
			case 75u:	return BR_75;
			case 110u:	return BR_110;
			case 134u:	return BR_134;
			case 150u:	return BR_150;
			case 200u:	return BR_200;
			case 300u:	return BR_300;

			case 600u:	return BR_600;
			case 1200u:	return BR_1200;
			case 1800u:	return BR_1800;
			case 2400u:	return BR_2400;
			case 4800u:	return BR_4800;
			case 9600u:	return BR_9600;
			case 19200u: return BR_19200;
			case 38400u: return BR_38400;

			case 57600u:	return BR_57600;
			case 115200u:	return BR_115200;
			case 230400u:	return BR_230400;

			case 460800u:	return BR_460800;
			case 500000u:	return BR_500000;
			case 576000u:	return BR_576000;
			case 921600u:	return BR_921600;

			case 1000000u:	return BR_1000000;
			case 1152000u:	return BR_1152000;
			case 1500000u:	return BR_1500000;
			case 2000000u:	return BR_2000000;
			case 2500000u:	return BR_2500000;
			case 3000000u:	return BR_3000000;
			case 3500000u:	return BR_3500000;
			case 4000000u:	return BR_4000000;

			default:
				break;
			}
			return BR_INVALID;
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
