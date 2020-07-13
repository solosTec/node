/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Sylko Olzscher
 *
 */

#ifndef NODE_SML_INTRINSICS_OBIS_FACTORY_HPP
#define NODE_SML_INTRINSICS_OBIS_FACTORY_HPP

#include <smf/sml/intrinsics/obis.h>
#include <cyng/intrinsics/sets.h>

namespace node
{
	namespace sml
	{
		/**
		 * Generate an OBIS code
		 */
		obis make_obis(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);

		/**
		 * @return a tuple of buffer_t objects
		 */
		cyng::tuple_t tuple_from_path(obis_path_t path);

		/**
		 * @return a vector of buffer_t objects
		 */
		cyng::vector_t vector_from_path(obis_path_t path);

		/**
		 * @brief converts a vector of objects into a obis path.
		 * The precondition is that all objects of type buffer_t.
		 */
		obis_path_t tuple_to_path(cyng::tuple_t tpl);

		/**
		 * Converts a vector of OBIS objects into an OBIS path.
		 * The object have be of type buffer, string or obis.
		 *
		 * @brief converts a vector of objects into a obis path.
		 */
		obis_path_t vector_to_path(cyng::vector_t const& vec);

		/**
		 * @brief converts a vector of objects into a obis path.
		 * The precondition is that all objects of type buffer_t.
		 */
		obis_path_t vector_to_path(std::vector<std::string> const& vec);

		/**
		 * Convert an OBIS path to a vector of strings.
		 * Each element of the path will be handled as separate entity.
		 */
		cyng::vector_t path_to_vector(obis_path_t path);

		/**
		 * @param translate if true all known OBIS codes will be substituted to
		 * human readable form.
		 * @return vector of strings
		 */
		std::vector<std::string> transform_to_str_vector(obis_path_t const&, bool translate);

		/**
		 * @param translate if true all known OBIS codes will be substituted to
		 * human readable form.
		 * @return vector of objects of type string
		 */
		cyng::vector_t transform_to_obj_vector(obis_path_t const&, bool translate);


	}	//	sml
}	//	node

//
//	macro to create OBIS codes without the 0x prefix
//

#define OBIS_CODE(p1, p2, p3, p4, p5, p6)	\
	node::sml::make_obis (0x##p1, 0x##p2, 0x##p3, 0x##p4, 0x##p5, 0x##p6)

#endif
