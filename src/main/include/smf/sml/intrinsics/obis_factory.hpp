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
		 * Generate an OBIS code from an existing code but change the storage field
		 * 
		 * @param s storage value
		 */
		obis make_obis(obis const&, std::uint32_t s);

		/**
		 * Generate an OBIS code from an existing code but change the storage field
		 * 
		 * @param q quantity value
		 * @param s storage value
		 */
		obis make_obis(obis const&, std::uint32_t q, std::uint32_t s);

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



	}	//	sml
}	//	node

//
//	macro to create OBIS codes without the 0x prefix
//

#define OBIS_CODE(p1, p2, p3, p4, p5, p6)	\
	node::sml::make_obis (0x##p1, 0x##p2, 0x##p3, 0x##p4, 0x##p5, 0x##p6)

#endif
