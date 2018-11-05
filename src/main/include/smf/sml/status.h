/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_STATUS_H
#define NODE_SML_STATUS_H


#include <cstdint>
#include <cyng/intrinsics/sets.h>

namespace node
{
	namespace sml
	{
		enum status_bits : std::uint64_t
		{
			//	always 1 - 0b0000000000000010
			STATUS_BIT_ON = (1ull << 1),

			//	fatal error - 0b0000000100000000
			STATUS_BIT_FATAL_ERROR = (1ull << 8),

			//	restart triggered by watchdog reset - 0b00000011000000000
			STATUS_BIT_RESET_BY_WATCHDOG = (1ull << 9),

			//	restart triggered by watchdog reset
			STATUS_BIT_RESTART = (1ull << 9),

			//	IP address is available (DHCP)
			STATUS_BIT_IP_ADDRESS_AVAILABLE = (1ull << 10),

			//	ethernet link is available
			STATUS_BIT_ETHERNET_AVAILABLE = (1ull << 11),

			//	authorized on IP-T server
			STATUS_BIT_AUTHORIZED_IPT = (1ull << 13),

			//	out of mememory
			STATUS_BIT_OUT_OF_MEMORY = (1ull << 14),

			//	 Service interface is available (Kundenschnittstelle)
			STATUS_BIT_SERVICE_IF_AVAILABLE = (1ull << 16),

			//	  extension interface is available (Erweiterungs-Schnittstelle)
			STATUS_BIT_EXT_IF_AVAILABLE = (1ull << 17),

			//	 Wireless M-Bus interface is available
			STATUS_BIT_MBUS_IF_AVAILABLE = (1ull << 18),

			//	PLC is available
			STATUS_BIT_PLC_AVAILABLE = (1ull << 19),

			//	time base is unsure
			STATUS_BIT_NO_TIMEBASE = (1ull << 32),
		};

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
			status();
			status(status const&);
			status& operator=(status const&);

			/**
			 * reset kernel
			 */
			void reset();
			void reset(std::uint32_t);

			/**
			 * @return status word
			 */
			operator std::uint64_t() const;

			void set_fatal_error(bool);
			void set_authorized(bool);
			void set_ip_address_available(bool);
			void set_ethernet_link_available(bool);
			void set_service_if_available(bool);
			void set_ext_if_available(bool);
			void set_mbus_if_available(bool);

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
			 * @return true if STATUS_BIT_MBUS_IF_AVAILABLE bit is set
			 */
			bool is_mbus_available() const;

			/**
			 * @return true if STATUS_BIT_PLC_AVAILABLE bit is set
			 */
			bool is_plc_available() const;

			/**
			 * @return true if STATUS_BIT_NO_TIMEBASE bit is set
			 */
			bool is_timebase_uncertain() const;


		private:
			bool is_set(status_bits) const;
			void set(status_bits);
			void remove(status_bits);

		private:
			std::uint64_t	word_;
		};

		/**
		 * Convert status bit map into an attribute map
		 * with enum status_bits as index.
		 */
		cyng::attr_map_t to_attr_map(status const&);
		cyng::param_map_t to_param_map(status const&);

	}	//	sml
}

#endif