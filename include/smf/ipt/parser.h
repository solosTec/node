/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Sylko Olzscher
 *
 */
#ifndef SMF_IPT_PARSER_H
#define SMF_IPT_PARSER_H

#include <smf/ipt.h>
#include <smf/ipt/scrambler.hpp>
#include <smf/ipt/scramble_key.h>
#include <smf/ipt/header.h>

#include <cyng/obj/intrinsics/buffer.h>

#include <algorithm>
#include <iterator>
#include <type_traits>

#include <boost/assert.hpp>


namespace smf
{
	namespace ipt	
	{
		class parser
		{
			using scrambler_t = scrambler<char, scramble_key::SIZE::value>;

			/**
			 * This enum stores the global state
			 * of the parser. For each state there
			 * are different helper variables mostly
			 * declared in the private section of this class.
			 */
			enum class state
			{
				STREAM,
				ESC,
				HEADER,
				DATA,
			} state_;

		public:
			parser(scramble_key const&);

			/**
			 * parse the specified range
			 */
			template < typename I >
			cyng::buffer_t read(I start, I end)
			{
				static_assert(std::is_same_v<char, typename std::iterator_traits<I>::value_type>, "data type char expected");

				cyng::buffer_t buffer;
				std::transform(start, end, std::back_inserter(buffer), [this](char c) {
					//
					//	Decode input stream
					//
					return this->put(scrambler_[c]);
				});

				post_processing();
				return buffer;
			}

			/**
			 * Client has to set new scrambled after login request
			 */
			void set_sk(scramble_key const&);

			/**
			 * Reset parser (default scramble key)
			 */
			void reset(scramble_key const&);

			/**
			 * Clear internal state and all callback function
			 */
			void clear();

		private:
			/**
			 * read a single byte and update
			 * parser state.
			 * Implements the state machine
			 */
			void put(char c);

			/**
			 * Probe if parsing is completed and
			 * inform listener.
			 */
			void post_processing();

			state state_stream(char c);
			state state_esc(char c);
			state state_header(char c);
			state state_data(char c);
			void read_command();

		private:
			scramble_key def_sk_;	//!<	system wide default scramble key

			/**
			 * Decrypting data stream
			 */
			scrambler_t	scrambler_;

			/**
			 * temporary data
			 */
			cyng::buffer_t	buffer_;

			//
			//	current header
			//
			header	header_;

		};
	}	//	ipt
}

#endif
