/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_SCRAMBLE_KEY_H
#define SMF_IPT_SCRAMBLE_KEY_H

#include <array>
#include <iostream>
#include <string>
#include <type_traits>

namespace smf {

	namespace ipt  {

		class scramble_key
		{
		public:
			using SIZE = std::integral_constant<std::size_t, 32>;
			using value_type = char;

			using key_type = std::array< value_type, SIZE::value >;

			constexpr static std::size_t size() noexcept
			{
				return SIZE::value;
			}

		public:
			scramble_key();
			scramble_key(key_type const&);
			scramble_key(scramble_key const&) = default;

			void swap(scramble_key&);

			std::ostream& dump(std::ostream&) const;
			std::string str() const;

			key_type const& key() const;

		public:
			constexpr static key_type default_scramble_key_ = { {
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
			0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
			0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04,
			0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00, 0x01,
		} };
			constexpr static key_type null_scramble_key_ = { {
			0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		} };

		private:
			key_type	key_;
		};

		/**
		 * Generate a scramble key with random values.
		 */
		scramble_key gen_random_sk();

	}	//	ipt
}

#endif
