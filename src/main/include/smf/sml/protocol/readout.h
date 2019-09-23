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

namespace node
{
	namespace sml
	{
		struct readout
		{
			readout();

			void reset();

			readout& set_trx(cyng::buffer_t const&);
			readout& set_value(std::string const&, cyng::object);

			/**
			 * overwrite existing values
			 */
			readout& set_value(cyng::param_t const&);
			readout& set_value(cyng::param_map_t&&);
			readout& set_value(obis, cyng::object);
			readout& set_value(obis, std::string, cyng::object);
			cyng::object get_value(std::string const&) const;
			cyng::object get_value(obis) const;
			std::string get_string(obis) const;

			std::string read_server_id(cyng::object obj);
			std::string read_client_id(cyng::object obj);

			std::string trx_;
			cyng::buffer_t	server_id_;
			cyng::buffer_t	client_id_;
			cyng::param_map_t values_;
		};
	}	//	sml
}

#endif