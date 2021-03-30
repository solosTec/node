/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_QUERY_H
#define SMF_IPT_QUERY_H

//#include <smf/ipt.h>
#include <cstdint>

namespace smf {

	namespace ipt	{

		/**
		 *	Definition of all IP-T processor instructions
		 */
		enum class query : std::uint32_t {

			//	application - protocol version (0xA000)
			PROTOCOL_VERSION = (1 << 0),	// 0b0000000000000001

			//	application - device firmware version (0xA001)
			FIRMWARE_VERSION = (1 << 1), // 0b0000000000000010

			//	application - device identifier (0xA003)
			DEVICE_IDENTIFIER = (1 << 2),	// 0b0000000000000100

			//	application - network status (0xA004)
			NETWORK_STATUS = (1 << 3),	// 0b0000000000001000

			//	application - IP statistic (0xA005)
			IP_STATISTIC = (1 << 4),	//	0b0000000000010000

			//	application - device authentification (0xA006)
			DEVICE_AUTHENTIFICATION = (1 << 5), // 0b0000000000100000

			//
			//	skip 6 
			//	skip 7
			//

			//	application - device time (0xA007)
			DEVICE_TIME = (1 << 8),	// 0b0000000100000000

			DEFAULT_VALUE = (FIRMWARE_VERSION | DEVICE_IDENTIFIER),
			ALL_VALUES = (PROTOCOL_VERSION
				| FIRMWARE_VERSION
				| DEVICE_IDENTIFIER
				| NETWORK_STATUS
				| IP_STATISTIC
				| DEVICE_AUTHENTIFICATION
				| DEVICE_TIME),
		};

		/**
		 *	@return name of the IP-T command
		 */
		const char* query_name(std::uint32_t);

		constexpr query to_code(std::uint32_t q) {
			return static_cast<query>(q);
		}

		/**
		 * Tests if the specified query bit is on
		 */
		constexpr bool test_bit(std::uint32_t v, query q) {
			return (v & static_cast<std::uint32_t>(q)) == static_cast<std::uint32_t>(q);
		}

	}	//	ipt
}	

#endif
