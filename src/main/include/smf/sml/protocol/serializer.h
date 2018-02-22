/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_SML_SERIALIZER_H
#define NODE_LIB_SML_SERIALIZER_H

#include <cyng/core/object_interface_fwd.h>
#include <ostream>

namespace node
{
	namespace sml
	{
		void serialize(std::ostream&, cyng::object const&);
	}
}
#endif