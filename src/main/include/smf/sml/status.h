/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_STATUS_H
#define NODE_SML_STATUS_H


#include <cyng/intrinsics/sets.h>
#include <cstdint>

namespace node
{
	namespace sml
	{
		enum status_bits : std::uint64_t
		{
			STAUS_BIT_0 = 0u,	// always zero
			//	always 1 - 0b0000000000000010
			STATUS_BIT_ON = (1ull << 1),

			STATUS_BIT_2 = (1ull << 2),	// always zero
			STATUS_BIT_3 = (1ull << 3),	// always zero
			STATUS_BIT_4 = (1ull << 4),	// always zero
			STATUS_BIT_5 = (1ull << 5),	// always zero
			STATUS_BIT_6 = (1ull << 6),	// always zero
			STATUS_BIT_7 = (1ull << 7),	// always zero

			//	fatal error - 0b0000000100000000
			STATUS_BIT_FATAL_ERROR = (1ull << 8),

			//	restart triggered by watchdog reset - 0b00000011000000000
			STATUS_BIT_RESET_BY_WATCHDOG = (1ull << 9),

			//	IP address is available (DHCP) 
			//	0 if address available
			STATUS_BIT_IP_ADDRESS_AVAILABLE = (1ull << 10),

			//	ethernet link is available
			//	0 if WAN is available
			STATUS_BIT_ETHERNET_AVAILABLE = (1ull << 11),

			//	0 if radio network is available (mobile network)
			STATUS_BIT_RADIO_AVAILABLE = (1ull << 12),

			//	not authorized on IP-T server
			//	0 if authorized
			STATUS_BIT_NOT_AUTHORIZED_IPT = (1ull << 13),

			//	out of mememory
			//	1 if OoM occured
			STATUS_BIT_OUT_OF_MEMORY = (1ull << 14),

			//	bit 15 is reserved
			STATUS_BIT_15 = 15u,	// always zero

			//	 Service interface is available (Kundenschnittstelle)
			//	1 if available
			STATUS_BIT_SERVICE_IF_AVAILABLE = (1ull << 16),

			//	extension interface is available (Erweiterungs-Schnittstelle)
			//	1 if available
			STATUS_BIT_EXT_IF_AVAILABLE = (1ull << 17),

			//	 wireless M-Bus interface is available
			//	1 if available
			STATUS_BIT_WIRELESS_MBUS_IF_AVAILABLE = (1ull << 18),

			//	PLC is available
			//	1 if available
			STATUS_BIT_PLC_AVAILABLE = (1ull << 19),

			//	wired M-Bus is available 
			//	1 if available (supported by SMF firmware only)
			STATUS_BIT_WIRED_MBUS_IF_AVAILABLE = (1ull << 20),

			STATUS_BIT_21 = (1ull << 21),	// always zero
			STATUS_BIT_22 = (1ull << 22),	// always zero
			STATUS_BIT_23 = (1ull << 23),	// always zero
			STATUS_BIT_24 = (1ull << 24),	// always zero
			STATUS_BIT_25 = (1ull << 25),	// always zero
			STATUS_BIT_26 = (1ull << 26),	// always zero
			STATUS_BIT_27 = (1ull << 27),	// always zero
			STATUS_BIT_28 = (1ull << 28),	// always zero
			STATUS_BIT_29 = (1ull << 29),	// always zero
			STATUS_BIT_30 = (1ull << 30),	// always zero
			STATUS_BIT_31 = (1ull << 31),	// always zero

			//	time base is unsure
			//	1 if unsafe
			STATUS_BIT_NO_TIMEBASE = (1ull << 32),
		};

		/**
		 * Define a base type to hold the status word information.
		 */
		using status_word_t = std::uint64_t;

		/**
		 * Manage status word for SML kernel
		 *	bit meaning
		 *	0	always 0
		 *	1	always 1
		 *	2-7	always 0
		 *	8	1 if fatal error was detected
		 *	9	1 if restart was triggered by watchdog reset
		 *	10	0 if IP address is available (DHCP)
			 - PSTN: Link with peer established
			 - GSM: Link with peer established
			 - GPRS: authorized on GPRS service
			 - LAN/DSL: IP address from DHCP or PPPoE available
		 *	11	0 if ethernet link / WAN / GSM network is available
		 *	12	always 0 (logged in GSM network)
		 *	13	0 if authorized on IP-T server
		 *	14	1 in case of out of memory
		 *	15	reserved - always 0
		 *	16	1 if Service interface is available (Kundenschnittstelle)
		 *	17	1 if extension interface is available (Ethernet Erweiterungs-Schnittstelle)
		 *	18	1 if Wireless M-Bus interface is available
		 *	19	1 if PLC is available
		 *	20-31	always 0
		 *	32	1 if time base is unsure

		 * 0x72202 = ‭0111 00[1]0 0010 0000 0010‬	- not authorized on IP-T master
		 * 0x70202 = ‭0111 00[0]0 0010 0000 0010‬	- authorized on IP-T master
		 * 0x62602 = ‭0111 00[1]0 0110 0000 0010‬
		 */
		class status
		{
		public:
			status(status_word_t&);
			status(status const&);

			status& operator=(status const&) = delete;

			/**
			 * reset status
			 */
			void reset();

			/**
			 * reset internal value (word)
			 *
			 * @return previous value
			 */
			std::uint64_t reset(std::uint64_t);

			/**
			 * @return status word
			 */
			operator std::uint64_t() const;
			std::uint64_t get() const;

			void set_fatal_error(bool);
			void set_authorized(bool);
			void set_ip_address_available(bool);
			void set_ethernet_link_available(bool);
			void set_service_if_available(bool);
			void set_ext_if_available(bool);
			void set_mbus_if_available(bool);
			void set_flag(status_bits, bool);

			/**
			 * convinience function for:
			 *
			 * @code
			 ! is_set(STATUS_BIT_AUTHORIZED_IPT);
			 * @endcode
			 *
			 * @return true if online and successful authorized at IP-T master
			 */
			bool is_authorized() const;

			/**
			 * @return true if STATUS_BIT_FATAL_ERROR bit is set
			 */
			bool is_fatal_error() const;

			/**
			 * @return true if STATUS_BIT_OUT_OF_MEMORY bit is set
			 */
			bool is_out_of_memory() const;

			/**
			 * @return true if STATUS_BIT_SERVICE_IF_AVAILABLE bit is set
			 */
			bool is_service_if_available() const;

			/**
			 * @return true if STATUS_BIT_EXT_IF_AVAILABLE bit is set
			 */
			bool is_ext_if_available() const;

			/**
			 * @return true if STATUS_BIT_WIRELESS_MBUS_IF_AVAILABLE bit is set
			 */
			bool is_wireless_mbus_available() const;

			/**
			 * @return true if STATUS_BIT_WIRED_MBUS_IF_AVAILABLE bit is set
			 */
			bool is_wired_mbus_available() const;

			/**
			 * @return true if STATUS_BIT_PLC_AVAILABLE bit is set
			 */
			bool is_plc_available() const;

			/**
			 * @return true if STATUS_BIT_NO_TIMEBASE bit is set
			 */
			bool is_timebase_uncertain() const;

			static status_word_t get_initial_value();

		private:
			bool is_set(status_bits) const;
			void set(status_bits);
			void remove(status_bits);

		private:
			status_word_t& word_;
		};

		/**
		 * Convert status bit map into an attribute map
		 * with enum status_bits as index.
		 */
		cyng::attr_map_t to_attr_map(status const&);
		cyng::param_map_t to_param_map(status const&);

		char const* get_status_name(status_bits sb);

	}	//	sml
}

#endif