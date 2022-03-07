/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_STATUS_H
#define SMF_SML_STATUS_H

#include <cstdint>
#include <cyng/obj/intrinsics/container.h>

namespace smf {

    namespace sml {

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
        enum class status_bit : status_word_t {
            _0 = 0u, // always zero

            //	always 1 - 0b0000000000000010
            _ON = 1ull << 1,

            _2 = 1ull << 2, // always zero
            _3 = 1ull << 3, // always zero
            _4 = 1ull << 4, // always zero
            _5 = 1ull << 5, // always zero
            _6 = 1ull << 6, // always zero
            _7 = 1ull << 7, // always zero

            //	fatal error - 0b0000000100000000
            FATAL_ERROR = 1ull << 8,

            //	restart triggered by watchdog reset - 0b00000011000000000
            RESET_BY_WATCHDOG = 1ull << 9,

            //	IP address is available (DHCP)
            //	0 if address available
            IP_ADDRESS_AVAILABLE = 1ull << 10,

            //	ethernet link is available
            //	0 if WAN is available
            ETHERNET_AVAILABLE = 1ull << 11,

            //	0 if radio network is available (mobile network)
            RADIO_AVAILABLE = 1ull << 12,

            //	not authorized on IP-T server
            //	0 if authorized
            NOT_AUTHORIZED_IPT = 1ull << 13,

            //	out of mememory
            //	1 if OoM occured
            OUT_OF_MEMORY = 1ull << 14,

            //	bit 15 is reserved
            _15 = 1ull << 15u, // always zero

            //	 Service interface is available (Kundenschnittstelle)
            //	1 if available
            SERVICE_IF_AVAILABLE = 1ull << 16,

            //	extension interface is available (Erweiterungs-Schnittstelle)
            //	1 if available
            EXT_IF_AVAILABLE = 1ull << 17,

            //	 wireless M-Bus interface is available
            //	1 if available
            WIRELESS_MBUS_IF_AVAILABLE = 1ull << 18,

            //	PLC is available
            //	1 if available
            PLC_AVAILABLE = 1ull << 19,

            //	wired M-Bus is available
            //	1 if available (supported by SMF firmware only)
            WIRED_MBUS_IF_AVAILABLE = 1ull << 20,

            _21 = 1ull << 21, // always zero
            _22 = 1ull << 22, // always zero
            _23 = 1ull << 23, // always zero
            _24 = 1ull << 24, // always zero
            _25 = 1ull << 25, // always zero
            _26 = 1ull << 26, // always zero
            _27 = 1ull << 27, // always zero
            _28 = 1ull << 28, // always zero
            _29 = 1ull << 29, // always zero
            _30 = 1ull << 30, // always zero
            _31 = 1ull << 31, // always zero

            //	time base is unsure
            //	1 if unsafe
            NO_TIMEBASE = 1ull << 32,
        };

        /**
         * convert to base type
         */
        constexpr status_word_t to_u64(status_bit sb) { return static_cast<status_word_t>(sb); }

        /**
         * @return the name of the status bit
         */
        char const *get_name(status_bit sb);

        /**
         * @return a usefull initial value for the status word
         */
        constexpr status_word_t get_initial_value() {
            return to_u64(status_bit::_ON) | to_u64(status_bit::RESET_BY_WATCHDOG) | to_u64(status_bit::EXT_IF_AVAILABLE) |
                   to_u64(status_bit::NOT_AUTHORIZED_IPT);
        }

        constexpr bool is_set(status_word_t w, status_bit e) { return (w & to_u64(e)) == to_u64(e); }

        constexpr status_word_t set(status_word_t w, status_bit e) { return w |= to_u64(e); }

        constexpr status_word_t remove(status_word_t w, status_bit e) { return w &= ~to_u64(e); }

        /**
         * Convert status bit map into an attribute map
         * with enum status_bits as index.
         */
        cyng::param_map_t to_param_map(status_word_t word);
    } // namespace sml
} // namespace smf
#endif
