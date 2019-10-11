/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_EVENT_H
#define NODE_SML_EVENT_H

/** @file event.h
 * 
 * OBIS code "81 81 C7 89 E2 FF" (OBIS_CLASS_EVENT) provides an u32 value that 
 * encodes an event type with informations about event type (enum event_type)
 * level and source.
 */

#include <cstdint>

namespace node
{
	namespace sml
	{

		std::uint32_t evt_timer();

		/**
		 * voltage recovery
		 */
		std::uint32_t evt_voltage_recovery();

		/**
		 * If possible lof this event in case of power outage.
		 */
		std::uint32_t evt_power_outage();

		/** @brief 0x00100001
		 *
		 * First entry after power recovery
		 */
		std::uint32_t evt_power_recovery();

		/** @brief 0x00100003
		 *
		 * Firmware successful activated
		 */
		std::uint32_t evt_firmware_activated();

		/** @brief 0x00100004
		 *
		 * Firmware not working
		 */
		std::uint32_t evt_firmware_failure();

		/** @brief 0x00100005
		 *
		 * Firmware hash error
		 */
		std::uint32_t evt_firmware_hash_error();

		/** @brief 0x00100006
		 *
		 * Firmware type error (wrong hardware)
		 */
		std::uint32_t evt_firmware_type_error();

		/** @brief 0x00100007
		 *
		 * Firmware upload OK
		 */
		std::uint32_t evt_firmware_upload_complete();

		/** @brief 0x00800000
		 *
		 * Timer - cyclic logbook entry
		 */
		std::uint32_t evt_timer();

		/** @brief 0x00800004
		 *
		 * cyclic reset of a communication module
		 */
		std::uint32_t evt_cyclic_reset();

		/** @brief 0x00800010
		 *
		 * set system time by user
		 */
		std::uint32_t evt_system_time_set();

		std::uint32_t evt_watchdog();
		std::uint32_t evt_push_succes();
		std::uint32_t evt_push_failed();
		std::uint32_t evt_ipt_connect();
		std::uint32_t evt_ipt_disconnect();
		std::uint32_t evt_npt_connect();
		std::uint32_t evt_npt_disconnect();

		/**
		 * @return event type
		 */
		std::uint32_t evt_get_type(std::uint32_t);

		/**
		 * @return event level
		 */
		std::uint32_t evt_get_level(std::uint32_t);

		/**
		 * @return event source
		 */
		std::uint32_t evt_get_source(std::uint32_t);

	}	//	sml
}

#endif