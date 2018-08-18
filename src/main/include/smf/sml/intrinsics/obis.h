/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_INTRINSICS_OBIS_H
#define NODE_SML_INTRINSICS_OBIS_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <smf/sml/defs.h>
#include <array>
#include <cstdint>
#include <algorithm>
#include <boost/config.hpp>

namespace node
{
	namespace sml
	{
		class obis
		{
		public:
			typedef std::array< std::uint8_t, 6 >	data_type;

		public:
			obis();
			obis(octet_type const&);
			obis(obis const&);
			obis(data_type const&);
			obis(std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t);

			void swap(obis&);

			/**
			 * Set all values to zero
			 */
			void clear();

			//	comparison
			bool equal(obis const&) const;
			bool less(obis const&) const;
			bool is_matching(obis const&) const;
			bool is_matching(std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t) const;

			/**
			 * Test if the first 5 specified bytes are matching.
			 * 
			 * @return a pair consisting of an value of the last element and a bool denoting whether the pattern is matching.
			 */
			std::pair<std::uint8_t, bool> is_matching(std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t) const;

			/**
			 * partial match
			 */
			bool match(std::initializer_list<std::uint8_t> il) const;

			/**
			 * assignment
			 */
			obis& operator=(obis const&);

			/**
			 *	@return 6
			 */
			//BOOST_CONSTEXPR
			inline std::size_t size() const {
				return std::tuple_size< data_type >::value;
			}

			/**
			 *	@return value group A
			 */
			std::uint32_t get_medium() const;
			const char* get_medium_name() const;
			bool is_abstract() const;

			/**
			 *	@return value group B
			 */
			std::uint32_t get_channel() const;
			const char* get_channel_name() const;

			/**
			 *	@return value group C
			 */
			std::uint32_t get_indicator() const;

			/**
			 *	@return value group D
			 */
			std::uint32_t get_mode() const;

			/**
			 *	A value of 0xFF means "don't care".
			 *	@return value group E
			 */
			std::uint32_t get_quantities() const;

			/**
			 *	A value of 0xFF means "don't care".
			 *	@return value group F
			 */
			std::uint32_t get_storage() const;

			/**
			 *	@return true if OBIS code has no global definition.
			 */
			bool is_private() const;

			/**
			 * @return true if all elements are zero.
			 */
			bool is_nil() const;

			/**
			 * @return true OBIS code contain a physical unit (That is not UNIT_RESERVED, OTHER_UNIT or COUNT).
			 */
			bool is_physical_unit() const;

			/*
			 *	Create a buffer containing all 6 bytes of
			 *	the OBIS value.
			 */
			octet_type to_buffer() const;

		private:
			data_type	value_;
		};

		// comparisons
	 	bool operator== (const obis&, const obis&);
	 	bool operator< (const obis&, const obis&);
	 	bool operator!= (const obis&, const obis&);
	 	bool operator> (const obis&, const obis&);
	 	bool operator<= (const obis&, const obis&);
	 	bool operator>= (const obis&, const obis&);
	 
	 	// global swap()
	 	void swap(obis&, obis&);

		//
		//	define an OBIS path
		//
		using obis_path = std::vector<obis>;


	}	//	sml
}	//	node

#endif	//	NODE_SML_INTRINSICS_OBIS_H
