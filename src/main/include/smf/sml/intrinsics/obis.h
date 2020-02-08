/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_SML_INTRINSICS_OBIS_H
#define NODE_SML_INTRINSICS_OBIS_H

#include <smf/sml/defs.h>
#include <cyng/util/slice.hpp>
#include <cyng/io/swap.h>

#include <array>
#include <cstdint>
#include <algorithm>
#include <functional>	//	hash
#include <boost/config.hpp>

namespace node
{
	namespace sml
	{
		class obis
		{
		public:
			typedef std::array< std::uint8_t, 6 >	data_type;

			enum value_group {
				VG_MEDIUM = 0,		//	A
				VG_CHANNEL = 1,		//	B
				VG_INDICATOR = 2,	//	C - metric
				VG_MODE = 3,		//	D - measurement mode
				VG_QUANTITY = 4,	//	E - tariff
				VG_STORAGE = 5		//	F
			};

		public:
			constexpr obis()
				: value_{ 0 }
			{}

			/**
			 * buffer must have the same size as data_type (6 bytes).
			 */
			obis(octet_type const& buffer);
			obis(obis const&);
			obis(data_type const&);
			constexpr obis(std::uint8_t a
				, std::uint8_t b
				, std::uint8_t c
				, std::uint8_t d
				, std::uint8_t e
				, std::uint8_t f)
				: value_{ a, b, c, d, e, f }
			{}

			void swap(obis&);

			/**
			 * Set all values to zero
			 */
			void clear();

			//	comparison
			bool equal(obis const&) const;

			/**
			 * lexicographical comparison
			 */
			bool less(obis const&) const;

			/** @brief Same as equal()
			 */
			bool is_matching(obis const&) const;

			/** @brief Compare first 4 elements
			 */
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
			constexpr std::size_t size() const {
				return std::tuple_size< data_type >::value;
			}

			/**
			 * @return the internal data array
			 */
			inline data_type const& get_data() const {
				return value_;
			}

			/**
			 *	@return value group A
			 */
			std::uint32_t get_medium() const;
			const char* get_medium_name() const;

			/**
			 * @return true if medium is 0.
			 */
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
			const char* get_indicator_name() const;

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
			 * @return a combination of the last two values (group E and F)
			 */
			std::uint16_t get_number() const;

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

			/**
			 *	Create a buffer containing all 6 bytes of
			 *	the OBIS value.
			 */
			octet_type to_buffer() const;

			/**
			 * Create a hex string without spaces.
			 *
			 * @see node::sml::to_hex(obis const&);
			 */
			std::string to_str() const;

			/**
			 * Calculate a hash value of 64 bits
			 */
			std::size_t hash() const;

			/**
			 *	Fill the first 6 bytes from a u64 value with the OBIS groups
			 */
			constexpr std::uint64_t to_uint64() const
			{
				//	use only the lower 6 bytes
				auto const n = cyng::slicer<std::uint64_t, 0u>(value_) & 0x0000FFFFFFFFFFFF;
				return cyng::swap_num(n * 0x10000);
			}

		private:
			const char* get_indicator_name_abstract() const;
			const char* get_indicator_name_electricity() const;
			const char* get_indicator_name_hca() const;
			const char* get_indicator_name_gas() const;

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

		/**
		 *	define an OBIS path
		 */
		using obis_path = std::vector<obis>;

	}	//	sml
}	//	node

namespace std
{
	template<>
	struct hash<node::sml::obis>
	{
		size_t operator()(node::sml::obis const& sk) const noexcept;
	};

	template<>
	struct equal_to<node::sml::obis>
	{
		using result_type = bool;
		using first_argument_type = node::sml::obis;
		using second_argument_type = node::sml::obis;

		bool operator()(node::sml::obis const& t1, node::sml::obis const& t2) const noexcept;
	};

	template<>
	struct less<node::sml::obis>
	{
		using result_type = bool;
		using first_argument_type = node::sml::obis;
		using second_argument_type = node::sml::obis;

		bool operator()(node::sml::obis const& t1, node::sml::obis const& t2) const noexcept;
	};

}

#endif	//	NODE_SML_INTRINSICS_OBIS_H
