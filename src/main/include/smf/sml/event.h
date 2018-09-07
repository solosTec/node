/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_EVENT_H
#define NODE_SML_EVENT_H


#include <cstdint>

namespace node
{
	namespace sml
	{

		std::uint32_t evt_timer();
		std::uint32_t evt_power_on();
		std::uint32_t evt_power_outage();
		std::uint32_t evt_watchdog();
		std::uint32_t evt_push_succes();
		std::uint32_t evt_push_failed();
		std::uint32_t evt_ipt_connect();
		std::uint32_t evt_ipt_disconnect();
		std::uint32_t evt_npt_connect();
		std::uint32_t evt_npt_disconnect();

	}	//	sml
}

#endif