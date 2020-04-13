/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_SRV_ID_IO_H
#define NODE_SML_SRV_ID_IO_H

 /** @file srv_id.h
  * Dealing with server IDs
  */

#include <cyng/intrinsics/buffer.h>
#include <cyng/intrinsics/mac.h>
#include <ostream>

namespace node
{
	namespace sml
	{
		/**
		 * example: 02-e61e-03197715-3c-07
		 * format is:
		 * 1 byte: Kennung (0x01 == wireless or 0x02 == wires)
		 * 2 bytes: manufacturer code
		 * 4 bytes: ident code
		 * 1 byte: version
		 * 1 byte: medium
		 */
		bool is_mbus(std::string const&);

		bool is_mbus_wired(cyng::buffer_t const&);
		bool is_mbus_radio(cyng::buffer_t const&);

		/**
		 * short: 
		 */
		bool is_w_mbus(cyng::buffer_t const&);

		/**
		 * example: 05823740
		 */
		bool is_serial(std::string const&);
		bool is_serial(cyng::buffer_t const&);

		bool is_gateway(cyng::buffer_t const&);
		bool is_dke_1(cyng::buffer_t const&);
		bool is_dke_2(cyng::buffer_t const&);

		/**
		 * switches are outdated
		 */
		bool is_switch(cyng::buffer_t const&);

		enum srv_type : std::uint32_t {
			SRV_MBUS_WIRED,	//	M-Bus (long)
			SRV_MBUS_RADIO,	//	M-Bus (long)
			SRV_W_MBUS,		//	wireless M-Bus (short)
			SRV_SERIAL,
			SRV_GW,
			SRV_BCD,	//	Rhein-Energie
			SRV_EON,	//	e-on
			SRV_DKE_1,	//	E DIN 43863-5:2010-02
			SRV_IMEI,	//	IMEI
			SRV_RWE,	//	RWE
			SRV_DKE_2,	//	E DIN 43863-5:2010-07
			SRV_SWITCH,	//	outdated
			SRV_OTHER
		};

		/**
		 * @return server type a compact enum
		 */
		srv_type get_srv_type(cyng::buffer_t const&);

		/**
		 * Find a textual representation for all server types
		 */
		void serialize_server_id(std::ostream& os, cyng::buffer_t const&);
		std::string from_server_id(cyng::buffer_t const&);

		/** @brief parser for server IDs
		 *
		 * Accept a server ID like 02-e61e-03197715-3c-07
		 * and convert it into a buffer.
		 */
		cyng::buffer_t from_server_id(std::string const&);

		/**
		 * Extract serial number from M-Bus or other formats
		 */
		std::string get_serial(cyng::buffer_t const&);
		std::string get_serial(std::string const&);

		/**
		 * Extract manufacturer code from M-Bus format.
		 */
		std::uint16_t get_manufacturer_code(std::string const&);
		std::uint16_t get_manufacturer_code(cyng::buffer_t const&);

		/**
		 * Extract medium code from M-Bus format.
		 */
		std::uint8_t get_medium_code(cyng::buffer_t const&);

		/**
		 * Extract version from M-Bus format.
		 */
		std::uint8_t get_version(cyng::buffer_t const&);

		/**
		 * Build a server ID for a gateway by inserting 0x05 in front 
		 * of the buffer.
		 */
		cyng::buffer_t to_gateway_srv_id(cyng::mac48);

	}	//	sml
}	//	node


#endif	
