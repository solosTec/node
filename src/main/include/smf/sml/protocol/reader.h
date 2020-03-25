/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_READER_H
#define NODE_SML_READER_H


#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis.h>
#include <smf/sml/units.h>
#include <smf/sml/protocol/readout.h>
#include <cyng/vm/context.h>
#include <cyng/intrinsics/sets.h>
#include <cyng/object.h>

#include <boost/uuid/uuid.hpp>

namespace node
{
	namespace sml
	{
		namespace reader
		{
			/**
			 * read a complete SML message
			 */
			cyng::vector_t read(cyng::tuple_t const&);
			cyng::vector_t read(cyng::tuple_t const&, boost::uuids::uuid);
			cyng::vector_t read_msg(cyng::tuple_t::const_iterator pos, cyng::tuple_t::const_iterator end);



		}
	}	//	sml
}

#endif