/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */

#include <smf/sml/event.h>

namespace smf
{
	namespace sml
	{
		enum event_nr
		{
			EVT_TIMER = 0,
			EVT_POWER_ON = 1,
			EVT_POWER_OUTAGE = 2,
			EVT_ACTIVATE_FW = 3,
			EVT_PERIODIC_RESET = 4,
			EVT_WATCHDOG = 5,
			EVT_SYNC_TOKEN_GENERATED = 6,
			EVT_SYNC_TOKEN_PASSED = 7,
			EVT_SUCCESS = 8,	//	push, WAN, ethernet
			EVT_FAILURE = 9,	//	push, WAN, ethernet
			EVT_NETWORK_LOGIN = 10,	//	GSM, PLC
			EVT_NETWORK_LOGOUT = 11,	//	GSM, PLC
			EVT_NETWORK_IPT = 14	//	IP-T
		};

		/**
		 * bits 31-24 source (8 bit)
		 * bits 23-20 level (4 bit)
		 * bits 29-8 reserved (12 bit)
		 * bits 7-0 event (8 bit)
		 */
		constexpr std::uint32_t make_op_event(std::uint8_t source, std::uint8_t level, std::uint8_t evt)
		{
			std::uint32_t op = evt << 24;
			op |= (level & 0x04) << 8;
			op |= source;
			return op;
		}

		std::uint32_t evt_timer() {
			return make_op_event(0x00, 0x08, 0x00);
		}
		std::uint32_t evt_voltage_recovery() {
			return make_op_event(0x00, 0x01, 0x23);
		}
		std::uint32_t evt_power_outage() {
			return make_op_event(0x00, 0x01, 0x02);
		}
		std::uint32_t evt_power_recovery() {
			return make_op_event(0x00, 0x01, 0x01);
		}
		std::uint32_t evt_firmware_activated() {
			return make_op_event(0x00, 0x01, 0x03);
		}
		std::uint32_t evt_firmware_failure() {
			return make_op_event(0x00, 0x01, 0x04);
		}
		std::uint32_t evt_firmware_hash_error() {
			return make_op_event(0x00, 0x01, 0x05);
		}
		std::uint32_t evt_firmware_type_error() {
			return make_op_event(0x00, 0x01, 0x06);
		}
		std::uint32_t evt_firmware_upload_complete() {
			return make_op_event(0x00, 0x01, 0x07);
		}
		std::uint32_t evt_cyclic_reset() {
			return make_op_event(0x00, 0x80, 0x04);
		}
		std::uint32_t evt_system_time_set() {
			return make_op_event(0x00, 0x08, 0x10);
		}
		std::uint32_t evt_watchdog() {
			return make_op_event(0x00, 0x08, 0x05);
		}
		std::uint32_t evt_push_succes() {
			return make_op_event(0x08, 0x08, 0x00);
		}
		std::uint32_t evt_push_failed()	{
			return make_op_event(0x09, 0x08, 0x00);
		}
		std::uint32_t evt_ipt_connect() {
			return make_op_event(0x49, 0x07, 0x0A);
		}
		std::uint32_t evt_ipt_disconnect() {
			return make_op_event(0x49, 0x07, 0x0E);
		}
		std::uint32_t evt_npt_connect() {
			return make_op_event(0x4A, 0x07, 0x0A);
		}
		std::uint32_t evt_npt_disconnect() {
			return make_op_event(0x4A, 0x07, 0x0E);
		}


	}	//	sml
}

