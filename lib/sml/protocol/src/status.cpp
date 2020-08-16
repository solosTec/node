/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#include <smf/sml/status.h>
#include <cyng/factory/set_factory.h>

namespace node
{
	namespace sml
	{

		status::status(status_word_t& word)
			: word_(word)
		{}

		status::status(status const& other)
			: word_(other.word_)
		{}

		void status::reset()
		{
			//
			//	reset status word
			//
			word_ = get_initial_value();
		}

		status_word_t status::get_initial_value()
		{
			status_word_t w{ 0 };
			w |= STATUS_BIT_ON;	//!< bit 1 is always ON
			w |= STATUS_BIT_RESET_BY_WATCHDOG;
			w |= STATUS_BIT_EXT_IF_AVAILABLE;	//!< 1 if NOT available
			w |= STATUS_BIT_NOT_AUTHORIZED_IPT;	//!< 1 if NOT authorized
			return w;
		}

		std::uint64_t status::reset(std::uint64_t word)
		{
			std::swap(word_, word);
			return word;
		}

		status::operator std::uint64_t() const
		{
			return get();
		}

		std::uint64_t status::get() const
		{
			return word_;
		}

		void status::set_fatal_error(bool b)
		{
			set_flag(STATUS_BIT_FATAL_ERROR, b);
		}

		void status::set_authorized(bool b)
		{
			//	0 if authorized
			set_flag(STATUS_BIT_NOT_AUTHORIZED_IPT, !b);
		}

		void status::set_ip_address_available(bool b)
		{
			//	0 if IP address is available (DHCP)
			set_flag(STATUS_BIT_IP_ADDRESS_AVAILABLE, b);
		}

		void status::set_ethernet_link_available(bool b)
		{
			//	0 if ethernet link / GSM network is available
			set_flag(STATUS_BIT_ETHERNET_AVAILABLE, b);
		}

		void status::set_service_if_available(bool b)
		{
			set_flag(STATUS_BIT_SERVICE_IF_AVAILABLE, b);
		}

		void status::set_ext_if_available(bool b)
		{
			set_flag(STATUS_BIT_EXT_IF_AVAILABLE, b);
		}

		void status::set_mbus_if_available(bool b)
		{
			set_flag(STATUS_BIT_WIRELESS_MBUS_IF_AVAILABLE, b);
		}

		void status::set_flag(status_bits e, bool b)
		{
			(b) ? set(e) : remove(e);
		}


		bool status::is_authorized() const
		{
			return !is_set(STATUS_BIT_NOT_AUTHORIZED_IPT);
		}

		bool status::is_fatal_error() const
		{
			return is_set(STATUS_BIT_FATAL_ERROR);
		}

		bool status::is_out_of_memory() const
		{
			return is_set(STATUS_BIT_OUT_OF_MEMORY);
		}

		bool status::is_service_if_available() const
		{
			return is_set(STATUS_BIT_SERVICE_IF_AVAILABLE);
		}

		bool status::is_ext_if_available() const 
		{
			return is_set(STATUS_BIT_EXT_IF_AVAILABLE);
		}

		bool status::is_wireless_mbus_available() const
		{
			return is_set(STATUS_BIT_WIRELESS_MBUS_IF_AVAILABLE);
		}

		bool status::is_wired_mbus_available() const
		{
			return is_set(STATUS_BIT_WIRED_MBUS_IF_AVAILABLE);
		}

		bool status::is_plc_available() const
		{
			return is_set(STATUS_BIT_PLC_AVAILABLE);
		}

		bool status::is_timebase_uncertain() const
		{
			return is_set(STATUS_BIT_NO_TIMEBASE);
		}

		bool status::is_set(status_bits e) const
		{
			return (word_ & e) == e;
		}

		void status::set(status_bits e)
		{
			word_ |= e;
		}

		void status::remove(status_bits e)
		{
			word_ &= ~e;
		}

		cyng::attr_map_t to_attr_map(status const& word)
		{
			return cyng::attr_map_factory
                (sml::STATUS_BIT_NOT_AUTHORIZED_IPT, word.is_authorized())
				(sml::STATUS_BIT_FATAL_ERROR, word.is_fatal_error())
				(sml::STATUS_BIT_OUT_OF_MEMORY, word.is_out_of_memory())
				(sml::STATUS_BIT_SERVICE_IF_AVAILABLE, word.is_service_if_available())
				(sml::STATUS_BIT_EXT_IF_AVAILABLE, word.is_ext_if_available())
				(sml::STATUS_BIT_WIRELESS_MBUS_IF_AVAILABLE, word.is_wireless_mbus_available())
				(sml::STATUS_BIT_PLC_AVAILABLE, word.is_plc_available())
				//	doesn't fit into size_t on 32 bit system
				(sml::STATUS_BIT_NO_TIMEBASE, word.is_timebase_uncertain())
				;
		}

		cyng::param_map_t to_param_map(status const& word)
		{
			return cyng::param_map_factory
				("AUTHORIZED_IPT", word.is_authorized())
				("FATAL_ERROR", word.is_fatal_error())
				("OUT_OF_MEMORY", word.is_out_of_memory())
				("SERVICE_IF_AVAILABLE", word.is_service_if_available())
				("EXT_IF_AVAILABLE", word.is_ext_if_available())
				("WIRELESS_BUS_IF_AVAILABLE", word.is_wireless_mbus_available())	//	wireless
				("WIRED_MBUS_IF_AVAILABLE", word.is_wired_mbus_available())	//	wired
				("PLC_AVAILABLE", word.is_plc_available())
				("NO_TIMEBASE", word.is_timebase_uncertain())
				;
		}

		char const* get_status_name(status_bits sb) {

			switch (sb) {
			case STAUS_BIT_0:	return "0";
			case STATUS_BIT_ON:	return "ON";
			case STATUS_BIT_2:	return "2";
			case STATUS_BIT_3:	return "3";
			case STATUS_BIT_4:	return "4";
			case STATUS_BIT_5:	return "5";
			case STATUS_BIT_6:	return "6";
			case STATUS_BIT_7:	return "7";
			case STATUS_BIT_FATAL_ERROR: return "FATAL_ERROR";
			case STATUS_BIT_RESET_BY_WATCHDOG:	return "RESET-BY-WATCHDOG";
			case STATUS_BIT_IP_ADDRESS_AVAILABLE:	return "IP-ADDRESS-AVAILABLE";
			case STATUS_BIT_ETHERNET_AVAILABLE:	return "ETHERNET-AVAILABLE";
			case STATUS_BIT_RADIO_AVAILABLE:	return "RADIO-AVAILABLE";
			case STATUS_BIT_NOT_AUTHORIZED_IPT:	return "NOT-AUTHORIZED-IPT";
			case STATUS_BIT_OUT_OF_MEMORY:	return "OUT-OF-MEMORY";
			case STATUS_BIT_15: return "15";
			case STATUS_BIT_SERVICE_IF_AVAILABLE:	return "SERVICE-IF-AVAILABLE";
			case STATUS_BIT_EXT_IF_AVAILABLE:	return "EXT-IF-AVAILABLE";
			case STATUS_BIT_WIRELESS_MBUS_IF_AVAILABLE:	return "WIRELESS-MBUS-IF-AVAILABLE";
			case STATUS_BIT_PLC_AVAILABLE:	return "PLC-AVAILABLE";
			case STATUS_BIT_WIRED_MBUS_IF_AVAILABLE: return "WIRED-MBUS-IF-AVAILABLE";
			case STATUS_BIT_21:	return "21";
			case STATUS_BIT_22:	return "22";
			case STATUS_BIT_23:	return "23";
			case STATUS_BIT_24:	return "24";
			case STATUS_BIT_25:	return "25";
			case STATUS_BIT_26:	return "26";
			case STATUS_BIT_27:	return "27";
			case STATUS_BIT_28:	return "28";
			case STATUS_BIT_29:	return "29";
			case STATUS_BIT_30:	return "30";
			case STATUS_BIT_31:	return "31";
			case STATUS_BIT_NO_TIMEBASE:	return "NO-TIMEBASE";
			default:
				break;
			}

			return "-";
		}

	}	//	sml
}

