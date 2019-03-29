/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Sylko Olzscher
 *
 */

#ifndef NODE_IPT_SCRAMBLE_KEY_H
#define NODE_IPT_SCRAMBLE_KEY_H

#include <array>
#include <functional>	//	hash

namespace node
{
	namespace ipt
	{
		class scramble_key
		{
		public:
			enum { SCRAMBLE_KEY_LENGTH = 32 };

			typedef std::array< char, SCRAMBLE_KEY_LENGTH >	key_type;
			typedef key_type::value_type		key_value_type;		//	char
			typedef key_type::iterator			key_iterator;		//	char*
			typedef key_type::const_iterator	key_const_iterator;	//	const char*
			typedef key_type::size_type			key_size_type;

		public:
			scramble_key();
			scramble_key(key_type const&);
			scramble_key(scramble_key const&);

			void swap(scramble_key&);

			//	comparison
			bool equal(scramble_key const&) const;
			bool less(scramble_key const&) const;

			scramble_key& operator=(scramble_key const&);

			static std::size_t size();

			std::ostream& dump(std::ostream&) const;
			std::string str() const;

			key_type const& key() const;

		public:
			static const key_type default_scramble_key_;
			static const key_type null_scramble_key_;

		private:
			key_type	value_;
		};

		/**
		 * Generate a scramble key with random values.
		 */
		scramble_key gen_random_sk();

	}	//	ipt
}	//	node

#include <cyng/intrinsics/traits.hpp>
#include <cyng/intrinsics/traits/tag.hpp>

namespace cyng 
{
	namespace traits
	{
		template <>
		struct type_tag<node::ipt::scramble_key>
		{
			using type = node::ipt::scramble_key;
			using tag = std::integral_constant<std::size_t, 
#if defined(__CPP_SUPPORT_N2347)
				static_cast<std::size_t>(traits::predef_type_code::PREDEF_SK)
#else
				PREDEF_SK
#endif
			>;

#if defined(__CPP_SUPPORT_N2235)
			constexpr static char name[] = "sk";
#else
			const static char name[];
#endif
		};

		template <>
		struct reverse_type < 
#if defined(__CPP_SUPPORT_N2347)
			static_cast<std::size_t>(traits::predef_type_code::PREDEF_SK)
#else
			PREDEF_SK
#endif
		>
		{
			using type = node::ipt::scramble_key;
		};
	}
}

namespace std
{
	template<>
	struct hash<node::ipt::scramble_key>
	{
		size_t operator()(node::ipt::scramble_key const& sk) const noexcept;
	};

	template<>
	struct equal_to<node::ipt::scramble_key>
	{
		using result_type = bool;
		using first_argument_type = node::ipt::scramble_key;
		using second_argument_type = node::ipt::scramble_key;

		bool operator()(node::ipt::scramble_key const& t1, node::ipt::scramble_key const& t2) const noexcept;
	};

	template<>
	struct less<node::ipt::scramble_key>
	{
		using result_type = bool;
		using first_argument_type = node::ipt::scramble_key;
		using second_argument_type = node::ipt::scramble_key;

		bool operator()(node::ipt::scramble_key const& t1, node::ipt::scramble_key const& t2) const noexcept;
	};

}

#endif	//	NODE_IPT_SCRAMBLE_KEY_H
