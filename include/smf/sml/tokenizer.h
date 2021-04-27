/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_SML_TOKENIZER_H
#define SMF_SML_TOKENIZER_H

#include <cyng/obj/intrinsics/buffer.h>
#include <smf/sml.h>
#include <smf/sml/crc16.h>

#include <iterator>
#include <type_traits>
#include <algorithm>

namespace smf {

	namespace sml {

		class parser;
		class unpack;

		/**
		 * Low level parser for SML data types
		 */
		class tokenizer
		{
			friend class parser;
			friend class unpack;

			/**
			 * This enum stores the global state
			 * of the parser. For each state there
			 * are different helper variables mostly
			 * declared in the private section of this class.
			 */
			enum class state
			{
				START,
				LENGTH,	//!<	check MSB
				LIST,	//!< different handling of length
				DATA
			} state_;

		public:
			using cb = std::function<void(sml_type, std::size_t, cyng::buffer_t)>;

		public:
			tokenizer(cb);

			/**
			 * parse the specified range
			 */
			template < typename I >
			void read(I start, I end)
			{
				static_assert(std::is_same_v<char, typename std::iterator_traits<I>::value_type>, "data type char expected");
				std::for_each(start, end, [this](char c)
					{
						//
						//	parse
						//
						this->put(c);
					});
			}

		private:

			/**
			 * read a single byte and update
			 * parser state.
			 * Implements the state machine
			 */
			void put(char);

			state start(char c);
			state start_data(char c);

			state state_length(char c);
			state state_list(char c);

			state state_data(char c);

		private:
			cb cb_;
			std::size_t length_;
			sml_type type_;
			cyng::buffer_t data_;

			/**
			 *	CRC designated by CCITT
			 */
			std::uint16_t	crc_;

		};
	}
}
#endif
