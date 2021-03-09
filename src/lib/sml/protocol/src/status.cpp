#include <smf/sml/status.h>

#ifdef _DEBUG_SML
#include <iostream>
#endif

#include <boost/assert.hpp>

namespace smf {
	namespace sml {

		char const* get_name(status_bit sb) {

			switch (sb) {
			case status_bit::FATAL_ERROR:	return "FATAL_ERROR";
			case status_bit::RESET_BY_WATCHDOG:	return "RESET_BY_WATCHDOG";
			case status_bit::IP_ADDRESS_AVAILABLE:	return "IP_ADDRESS_AVAILABLE";
			case status_bit::ETHERNET_AVAILABLE: return "ETHERNET_AVAILABLE";
			case status_bit::RADIO_AVAILABLE: return "RADIO_AVAILABLE";
			case status_bit::NOT_AUTHORIZED_IPT: return "NOT_AUTHORIZED_IPT";
			case status_bit::OUT_OF_MEMORY:	return "OUT_OF_MEMORY";
			case status_bit::SERVICE_IF_AVAILABLE: return "SERVICE_IF_AVAILABLE";
			case status_bit::EXT_IF_AVAILABLE:	return "EXT_IF_AVAILABLE";
			case status_bit::WIRELESS_MBUS_IF_AVAILABLE:	return "WIRELESS_MBUS_IF_AVAILABLE";
			case status_bit::PLC_AVAILABLE:	return "PLC_AVAILABLE";
			case status_bit::WIRED_MBUS_IF_AVAILABLE:	return "WIRED_MBUS_IF_AVAILABLE";
			case status_bit::NO_TIMEBASE:	return "NO_TIMEBASE";
			default:
				break;
			}
			return "status-bit";
		}

	}
}
