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

			/**
			 * read a SML message body
			 */
			cyng::vector_t read_choice(readout& ro, cyng::tuple_t const&);

			//
			//	helper
			//


			/**
			 * A SML parameter consists of a type (PROC_PAR_VALUE, PROC_PAR_TIME, ...) and a value.
			 * This functions read the parameter and creates an attribute that contains these
			 * two informations
			 * 
			 * Precondition is that the object contains a list/tuple with the a size of 2.
			 */
			cyng::attr_t read_parameter(cyng::object);

			/**
			 * SML defines 2 representations of time: TIME_TIMESTAMP and TIME_SECINDEX.
			 * This functions decodes the type and produces an object that contains the
			 * value. If the type is TIME_TIMESTAMP the return value is a std::chrono::system_clock::time_point.
			 * Otherwise the value is u32 value.
			 *
			 * Precondition is that the object contains a list/tuple with the a size of 2.
			 */
			cyng::object read_time(cyng::object);

			/**
			 * Take the elements of the buffer as ASCII code and build a string
			 *
			 * Precondition is that the object contains a binary buffer (cyng::buffer_t)
			 */
			cyng::object read_string(cyng::object);

			std::int8_t read_scaler(cyng::object);
			std::uint8_t read_unit(cyng::object);
			//std::uint8_t read_status(cyng::object);

			std::vector<obis> read_param_tree_path(cyng::object);

			cyng::param_t read_param_tree(std::size_t, cyng::object);
			cyng::param_t read_param_tree(std::size_t, cyng::tuple_t::const_iterator, cyng::tuple_t::const_iterator);

		}
	}	//	sml
}

#endif