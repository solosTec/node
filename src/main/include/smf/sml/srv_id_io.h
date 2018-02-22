/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_SRV_ID_IO_H
#define NODE_SML_SRV_ID_IO_H

#include <cyng/intrinsics/buffer.h>
#include <ostream>

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

	}	//	sml
}	//	node


#endif	
