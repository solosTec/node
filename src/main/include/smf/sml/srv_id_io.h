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
//#include <boost/uuid/uuid.hpp>

namespace node
{
	namespace sml
	{
		bool is_mbus(cyng::buffer_t const&);
		bool is_serial(cyng::buffer_t const&);
		bool is_gateway(cyng::buffer_t const&);

		void serialize_server_id(std::ostream& os, cyng::buffer_t const&);
		std::string from_server_id(cyng::buffer_t const&);

		std::string get_serial(cyng::buffer_t const&);

		/**
		 * Build a server ID for a gateway by inserting 0x05 in front 
		 * of the buffer.
		 */
		cyng::buffer_t to_gateway_srv_id(cyng::mac48);

		/**
		 *
		 */
		//cyng::buffer_t to_serial(boost::uuids::uuid);

	}	//	sml
}	//	node


#endif	
