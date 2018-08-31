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
		bool is_mbus(cyng::buffer_t const&);
		bool is_serial(cyng::buffer_t const&);
		bool is_gateway(cyng::buffer_t const&);

		enum srv_type : std::uint32_t {
			SRV_MBUS,
			SRV_SERIAL,
			SRV_GW,
			SRV_OTHER
		};

		/**
		 * @return server type a compact enum
		 */
		std::uint32_t get_srv_type(cyng::buffer_t const&);

		/**
		 * example: 02-e61e-03197715-3c-07
		 */
		bool is_mbus(std::string const&);

		/**
		 * example: 05823740
		 */
		bool is_serial(std::string const&);

		void serialize_server_id(std::ostream& os, cyng::buffer_t const&);
		std::string from_server_id(cyng::buffer_t const&);

		std::string get_serial(cyng::buffer_t const&);
		std::string get_serial(std::string const&);

		/**
		 * Build a server ID for a gateway by inserting 0x05 in front 
		 * of the buffer.
		 */
		cyng::buffer_t to_gateway_srv_id(cyng::mac48);

	}	//	sml
}	//	node


#endif	
