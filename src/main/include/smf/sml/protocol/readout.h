/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_LIB_READOUT_H
#define NODE_LIB_READOUT_H


#include <smf/sml/defs.h>
#include <smf/sml/intrinsics/obis.h>
#include <cyng/object.h>
#include <cyng/intrinsics/sets.h>

#include <boost/uuid/uuid.hpp>

namespace node
{
	namespace sml
	{
		struct readout
		{
			readout(boost::uuids::uuid);

			void reset(boost::uuids::uuid, std::size_t);

			readout& set_trx(cyng::buffer_t const&);
			readout& set_index(std::size_t);
			readout& set_value(std::string const&, cyng::object);
			readout& set_value(obis, cyng::object);
			readout& set_map(obis, std::string, cyng::object);
			cyng::object get_value(std::string const&) const;
			cyng::object get_value(obis) const;
			std::string get_string(obis) const;

			boost::uuids::uuid pk_;
			std::size_t idx_;	//!< message index
			std::string trx_;
			cyng::buffer_t	server_id_;
			cyng::buffer_t	client_id_;
			cyng::param_map_t values_;
		};
	}	//	sml
}

#endif