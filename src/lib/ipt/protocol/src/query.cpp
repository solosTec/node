/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/ipt/query.h>
#include <boost/assert.hpp>

namespace smf {

	namespace ipt	{

		const char* query_name(std::uint32_t c)
		{

			switch (to_code(c)) {

			case query::PROTOCOL_VERSION:	return "PROTOCOL_VERSION";

				//	application - device firmware version (0xA001)
				case query::FIRMWARE_VERSION:	return "PROTOCOL_VERSION";

				//	application - device identifier (0xA003)
				case query::DEVICE_IDENTIFIER:	return "DEVICE_IDENTIFIER";

				//	application - network status (0xA004)
				case query::NETWORK_STATUS:	return "NETWORK_STATUS";

				//	application - IP statistic (0xA005)
				case query::IP_STATISTIC:	return "IP_STATISTIC";

				//	application - device authentification (0xA006)
				case query::DEVICE_AUTHENTIFICATION:	return "DEVICE_AUTHENTIFICATION";

				//	application - device time (0xA007)
				case query::DEVICE_TIME:	return "DEVICE_TIME";

			default:
				BOOST_ASSERT_MSG(false, "undefined IP-T query");
				break;
			}

			return "undefined";
		}
	}	//	ipt	
}
